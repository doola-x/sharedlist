const activeTimers = []
const sleep = (ms) => new Promise(resolve => setTimeout(resolve, ms));
function throttle(func, delay) {
	let last = 0;
	return function(...args) {
		const now = new Date().getTime();
		if (now - last < delay) return;
		last = now;
		return func(...args);
	};
}
let tracklistHasNext = true;
let currentUser = null;
let welcomeHtml = '';
let spotifyPlayer = null;
let spotifyDeviceId = null;

function escapeHtml(str) {
	return String(str ?? '').replace(/[&<>"']/g, c => ({
		'&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;'
	}[c]));
}

// Fresh access token from our backend (it refreshes with the stored refresh token as needed).
async function fetchSpotifyToken() {
	const res = await fetch('/api/spotify_token', { cache: 'no-store' });
	if (!res.ok) return null;
	const data = await res.json();
	return data.access_token || null;
}

window.onSpotifyWebPlaybackSDKReady = async () => {
	// not signed in / no spotify link yet: skip, the player is created on demand later
	if (!(await fetchSpotifyToken())) return;
	initSpotifyPlayer();
};

function initSpotifyPlayer() {
	if (spotifyPlayer || typeof Spotify === 'undefined') return;
	spotifyPlayer = new Spotify.Player({
		name: 'sharedlist',
		getOAuthToken: async cb => { cb(await fetchSpotifyToken()); },
		volume: 0.5
	});
	spotifyPlayer.addListener('ready', ({ device_id }) => { spotifyDeviceId = device_id; });
	spotifyPlayer.addListener('not_ready', () => { spotifyDeviceId = null; });
	spotifyPlayer.addListener('initialization_error', e => console.error('spotify init error', e.message));
	spotifyPlayer.addListener('authentication_error', e => console.error('spotify auth error', e.message));
	spotifyPlayer.addListener('account_error', e => {
		console.error('spotify account error (Premium required)', e.message);
		const sel = document.getElementById('selected');
		if (sel) sel.textContent = 'spotify premium is required for playback';
	});
	spotifyPlayer.addListener('player_state_changed', renderPlayerState);
	spotifyPlayer.addListener('playback_error', e => console.error('spotify playback error', e.message));
	spotifyPlayer.connect();
}

function fmtTime(ms) {
	const s = Math.floor((ms || 0) / 1000);
	return `${Math.floor(s / 60)}:${String(s % 60).padStart(2, '0')}`;
}

let seekTimer = null;
let seeking = false;
let playbackPos = 0;
let playbackDur = 0;
let playbackStamp = 0;
let playbackPaused = true;

// With a single-uri context the SDK ends in a paused, position 0 state whose
// previous_tracks contains the track that just finished. Fire once per finish.
let advancedFrom = null;
function maybeAutoAdvance(state) {
	const cur = state.track_window.current_track;
	const ended = state.paused && state.position === 0
		&& state.track_window.previous_tracks.some(p => p.id === cur.id);
	if (ended && advancedFrom !== cur.id) {
		advancedFrom = cur.id;
		selectAdjacentTrack(1);
	} else if (!state.paused) {
		advancedFrom = null;
	}
}

function renderPlayerState(state) {
	const art = document.getElementById('art');
	if (!art) return; // not on the sharedlist page
	if (!state) return;
	maybeAutoAdvance(state);
	const t = state.track_window.current_track;
	const img = t.album.images[0];
	art.hidden = !img;
	if (img) art.src = img.url;
	document.getElementById('meta_title').textContent = t.name;
	document.getElementById('meta_artist').textContent = t.artists.map(a => a.name).join(', ');
	document.getElementById('meta_album').textContent = t.album.name;
	document.getElementById('btn_toggle').textContent = state.paused ? '\u25B6\uFE0E' : '\u23F8\uFE0E';

	playbackPos = state.position;
	playbackDur = state.duration;
	playbackStamp = Date.now();
	playbackPaused = state.paused;
	document.getElementById('seek').max = state.duration;
	document.getElementById('time_dur').textContent = fmtTime(state.duration);
	tickSeek();
	if (!seekTimer) seekTimer = setInterval(tickSeek, 500);
}

function tickSeek() {
	const seek = document.getElementById('seek');
	if (!seek) { clearInterval(seekTimer); seekTimer = null; return; }
	if (seeking) return;
	const pos = Math.min(playbackDur, playbackPos + (playbackPaused ? 0 : Date.now() - playbackStamp));
	seek.value = pos;
	document.getElementById('time_pos').textContent = fmtTime(pos);
}

// We start playback with a single uri, so the SDK has no queue to skip through.
// Instead "click" the neighbouring row, which goes through the normal play path.
async function selectAdjacentTrack(dir, retried = false) {
	const tbody = document.querySelector('.tracklist tbody');
	if (!tbody) return;
	const current = tbody.querySelector('.row-selected');
	const target = current
		? (dir > 0 ? current.nextElementSibling : current.previousElementSibling)
		: tbody.querySelector('tr.tracklist_item');
	if (target && target.classList.contains('tracklist_item')) {
		target.click();
		target.scrollIntoView({ block: 'nearest' });
		return;
	}
	// end of the loaded batch: nudge the scroll-loader and try once more
	if (dir > 0 && !retried && current && tracklistHasNext) {
		current.scrollIntoView({ block: 'nearest' });
		await sleep(1000);
		return selectAdjacentTrack(dir, true);
	}
}

function fmtRuntime(ms) {
	const mins = Math.round((ms || 0) / 60000);
	return mins >= 60 ? `${Math.floor(mins / 60)}h ${mins % 60}m` : `${mins}m`;
}

async function loadSharedlistInfo(sharedlistId) {
	try {
		const res = await fetch(`/api/sharedlist_info?sharedlist_id=${sharedlistId}`);
		if (res.status === 401) { handleUnauthorized(res); return; }
		if (!res.ok) return;
		const info = await res.json();
		const set = (id, text) => { const el = document.getElementById(id); if (el) el.textContent = text; };
		set('info_name', info.name || 'untitled sharedlist');
		set('info_description', info.description || '');
		set('info_owner', info.owner);
		set('info_origin_owner', info.origin_owner || '-');
		set('info_tracks', info.track_count);
		set('info_runtime', fmtRuntime(info.total_ms));
		// sqlite current_timestamp is UTC "YYYY-MM-DD HH:MM:SS"
		set('info_created', info.created_at ? info.created_at.split(' ')[0] : '');
		const cover = document.getElementById('info_cover');
		if (cover && info.image_url) { cover.src = info.image_url; cover.hidden = false; }
	} catch (err) {
		console.error('Error fetching sharedlist info:', err);
	}
}

function bindControls() {
	const on = (id, ev, fn) => document.getElementById(id).addEventListener(ev, fn);
	on('btn_prev', 'click', () => selectAdjacentTrack(-1));
	on('btn_next', 'click', () => selectAdjacentTrack(1));
	on('btn_toggle', 'click', () => spotifyPlayer && spotifyPlayer.togglePlay());
	on('seek', 'input', e => { seeking = true; document.getElementById('time_pos').textContent = fmtTime(+e.target.value); });
	on('seek', 'change', e => {
		seeking = false;
		if (spotifyPlayer) spotifyPlayer.seek(+e.target.value);
	});
	on('volume', 'input', e => spotifyPlayer && spotifyPlayer.setVolume(+e.target.value / 100));
	// the page was re-rendered: pull current state into the fresh DOM
	if (spotifyPlayer) spotifyPlayer.getCurrentState().then(renderPlayerState);
}

async function waitForDevice(timeoutMs = 5000) {
	const start = Date.now();
	while (!spotifyDeviceId && Date.now() - start < timeoutMs) await sleep(100);
	return spotifyDeviceId;
}

function loadContent(page, box) {
	activeTimers.forEach(clearTimeout);
	activeTimers.length = 0;
	return fetch(`components/${page}.html`)
	    .then(response => {
		if (!response.ok) {
		    throw new Error('Network response was not ok');
		}
		return response.text();
	    })
	    .then(html => {
		if (box == "app") {
			document.getElementById('app-content').innerHTML = html;
		}
		else if (box == "modal") {
			document.getElementById('modal-content').innerHTML = html;
			document.getElementById('modal-content').removeAttribute('style');
		}
	    })
	    .catch(error => {
		console.error('Error fetching the content:', error);
		document.getElementById('app-content').innerHTML = '<p>Error loading the page.</p>';
	    });
}

function fetchPlaylists() {
	return new Promise((resolve, reject) => {
		fetch('/api/spotify_playlists', { method: 'POST', credentials: 'same-origin' })
		.then(response => {
			if (response.status === 401) { handleUnauthorized(response); throw new Error('unauthorized'); }
			return response.json();
		})
		.then(data => {
			resolve(data);
		})
		.catch(err => {
			reject(err);
		});
	});
}

function makeSharedlist(type, id) {
	return new Promise((resolve, reject) => {
		const body = {
			origin_type: type,
			origin_id: id
		};
		fetch('/api/sharedlist', {
			method: 'POST',
			headers: {
				'Content-Type': 'application/json'
			},
			body: JSON.stringify(body)
		})
		.then(response => {
			if (response.status === 401) { handleUnauthorized(response); throw new Error('unauthorized'); }
			return response.json();
		})
		.then(data => {
			if (data.status !== 'success') throw new Error('could not create sharedlist');
			navigate(`#/sharedlist/${data.sharedlist_id}`);
			resolve(data);
		})
		.catch(err => {
			reject(err);
		});
	});
}

// A 401 is either "no session" (go sign in) or "signed in, but spotify isn't linked /
// its token can't be refreshed" (go to home, which has the connect button).
async function handleUnauthorized(res) {
	let body = {};
	try { body = await res.clone().json(); } catch (e) {}
	if ((body.status || body.error) === 'not signed in') {
		currentUser = null;
		showWelcome();
	} else {
		navigate('#/');
	}
}

function loadSharedlist(sharedlistId) {
	loadContent('home_sharedlist', 'app').then(async () => {
		const limit = 20;
		let loading = false;
		tracklistHasNext = true;
		bindControls();
		loadSharedlistInfo(sharedlistId);
		const tbody = document.querySelector('.tracklist tbody');
		tbody.addEventListener('click', (e) => {
			if (e.target.matches('input[type="checkbox"]')) return;

			const tr = e.target.closest('tr.tracklist_item');
			if (!tr) return;

			tbody.querySelectorAll('.row-selected').forEach(r => r.classList.remove('row-selected'));
			tr.classList.add('row-selected');

			const spotifyId = tr.querySelector('.spotify_id').textContent;
			const trackTitle = tr.querySelector('.track_title').textContent;
			const trackArtist = tr.querySelector('.track_artist').textContent;
			const trackAlbum = tr.querySelector('.track_album').textContent;
			const cachedArt = tr.dataset.image;
			const artEl = document.getElementById('art');
			if (artEl && cachedArt) { artEl.src = cachedArt; artEl.hidden = false; }
			renderMediaPlayer(spotifyId, trackTitle, trackArtist, trackAlbum); 
		});
		const tcontainer = document.querySelector('.tracklist-scroller');
		const handleScroll = throttle(async () => {
			let offset = tbody.children.length + 1;
			if (loading) {
				return;
			}
			if (!tracklistHasNext) {
				tcontainer.removeEventListener('scroll', handleScroll);
				return;
			}
			const scrollTop = tcontainer.scrollTop;     
			const scrollHeight = tcontainer.scrollHeight; 
			const clientHeight = tcontainer.clientHeight; 
			let delta = scrollHeight - scrollTop - clientHeight 
			if (delta <= 200) {
				await fetchBatch(offset);
			}
		}, 300);
		tcontainer.addEventListener('scroll', handleScroll); 
		tbody.innerHTML = '';

		async function fetchBatch(offset) {
			let rowIndex = tbody.children.length + 1;
			loading = true;
			try {
				const res = await fetch(`/api/sharedlist?sharedlist_id=${sharedlistId}&offset=${offset}&limit=${limit}`);
				if (res.status === 401) { handleUnauthorized(res); return false; }
				const data = await res.json();
				if (data["items"] == null || data["items"].length == 0) {
					loading = false;
					return false;
				}
				let tracks = data["items"];
				tracks.forEach(track => {
					const tr = document.createElement('tr');
					tr.className = 'tracklist_item';
					if (track.image_url) tr.dataset.image = track.image_url;
					tr.innerHTML = `<th scope="row">${rowIndex++}</th>` +
						`<td class="song_select" style="max-width: 20px;"><input type="checkbox"></td>` +
						`<td class="track_title">${escapeHtml(track.name)}</td>` +
						`<td class="track_artist">${escapeHtml(track.artists)}</td>` +
						`<td class="track_album">${escapeHtml(track.album)}</td>` +
						`<td class="spotify_id" style="display: none;">${escapeHtml(track.spotify_id)}</td>`;
					tbody.appendChild(tr);
				});
				loading = false;
				tracklistHasNext = data["has_next"];
			} catch(err) {
				console.error('Error fetching tracks:', err)
				loading = false;
			}

			return true;
		}

		
		let result = await fetchBatch(0);
		if (result === false) {
			const max = 5;
			let sleep_ms = 500;
			for (let i = 0; i < max; i++) {
				await sleep(sleep_ms);
				result = await fetchBatch(0);
				if (result === true) {
					return;
				}
				sleep_ms *= 2;
			}
		}
	});
}

async function renderMediaPlayer(originId, trackTitle, trackArtist, trackAlbum) {
	const title = document.getElementById("selected");
	title.textContent = `${trackTitle} - ${trackArtist}`;

	initSpotifyPlayer();
	if (!spotifyPlayer) {
		title.textContent = 'connect spotify to play tracks';
		return;
	}
	// browsers block audio until a user gesture; this runs inside the row click
	spotifyPlayer.activateElement();

	const deviceId = await waitForDevice();
	const token = await fetchSpotifyToken();
	if (!deviceId || !token) {
		title.textContent = 'player is not ready, try again in a moment';
		return;
	}
	const res = await fetch(`https://api.spotify.com/v1/me/player/play?device_id=${encodeURIComponent(deviceId)}`, {
		method: 'PUT',
		headers: { 'Authorization': `Bearer ${token}`, 'Content-Type': 'application/json' },
		body: JSON.stringify({ uris: [`spotify:track:${originId}`] })
	});
	if (!res.ok && res.status !== 204) {
		console.error('play failed', res.status, await res.text());
		title.textContent = `${trackTitle} - could not start playback`;
	}
}

function signIn(username, password) {
	return new Promise((resolve, reject) => {
		const user = {
			username: username,
			password: password
		};
		fetch('/api/signin', {
			method: 'POST',
			headers: {
				'Content-Type': 'application/json'
			},
			body: JSON.stringify(user)
		})
		.then(response => response.json())
		.then(data => {
			if (data.status == 'success') {
				resolve(data);
			} else {
				reject(data);
			}
		})
		.catch(err => {
			reject(err);
		});
	});
}

function signUp(username, password) {
	return fetch('/api/signup', {
		method: 'POST',
		headers: { 'Content-Type': 'application/json' },
		body: JSON.stringify({ username: username, password: password })
	})
	.then(response => response.json())
	.then(data => {
		if (data.status !== 'success') throw data;
		return signIn(username, password);
	});
}

function hideModal() {
	document.getElementById('modal-content').style.display = 'none';
}

function spotifyAuthLaunch() {
	window.location.href = "/api/spotify_signin";
}

function spawnSignUpIn(page) {
	fetch(`/components/${page}.html`)
		.then(response => {
			if (!response.ok) {
				throw new Error('Network response was not ok');
			}
			return response.text();
		})
		.then(html => {
			document.getElementById('app-content').innerHTML = html;
			if (page == 'signup') {
				document.getElementById('signupform').addEventListener('submit', function(e) {
					e.preventDefault();
					var username = document.getElementById('username').value;
					var password = document.getElementById('password').value;
					signUp(username, password)
					.then(data => afterSignIn())
					.catch(err => {
						loadContent('error_modal', 'modal');
					});
				});
			}
			if (page == "signin") {
				document.getElementById('signinform').addEventListener('submit', function(e) {
					e.preventDefault();
					var username = document.getElementById('username').value;
					var password = document.getElementById('password').value;
					signIn(username, password)
					.then(data => afterSignIn())
					.catch(err => {
						loadContent('error_modal', 'modal');
					});
				});
			}
		});	
}

function loadAuthd() {
	loadContent('home_auth', 'app');
	activeTimers.push(setTimeout(() => loadContent('hint_sharedlist_modal', 'modal'), 8000));
	fetchPlaylists()
		.then(data => {
			const lists = document.getElementById('lists');
			const images = document.getElementById('lists-images');
			lists.style.overflowY = "scroll"
			images.style.overflowY = "scroll";
			data.items.forEach(row => {
				if (!lists) {
					console.error(`Parent container '${parentId}' not found`);
					return;
				}
				const child = document.createElement('li');
				child.textContent = row.name.toLowerCase();
				child.style.transition = ".2s ease";
				child.style.cursor = "pointer";
				if (row.images[0].height/4 == 0 || row.images[0].width/4 == 0) return;
				const image = document.createElement('div');
				image.style.width = row.images[0].width/4 + "px";
				image.style.height = row.images[0].height/4 + "px";
				image.style.position = "relative";
				image.style.margin = "8px";
				image.style.border = "2px solid";
				image.style.display = "inline-block";
				image.style.backgroundImage = "url('" + row.images[0].url + "')";
				image.style.backgroundSize = "cover";
				image.style.transition = ".5s ease";
				image.style.cursor = "pointer";
				image.addEventListener('mouseover', function() {
					image.style.border = "3px solid white";
					image.style.width = row.images[0].width/3.9 + "px";
					image.style.height = row.images[0].height/3.9 + "px";
					child.style.color = "slategray";
				});
				image.addEventListener('mouseout', function() {
					image.style.border = "2px solid";
					image.style.width = row.images[0].width/4 + "px";
					image.style.height = row.images[0].height/4 + "px";
					child.style.color = "black";
				});
				image.addEventListener('click', () => { 
					makeSharedlist('spotify', row.id) 
				});
				child.addEventListener('mouseover', function() {
					image.style.border = "3px solid white";
					image.style.width = row.images[0].width/3.9 + "px";
					image.style.height = row.images[0].height/3.9 + "px";
					child.style.color = "slategray";
				});
				child.addEventListener('mouseout', function() {
					image.style.border = "2px solid";
					image.style.width = row.images[0].width/4 + "px";
					image.style.height = row.images[0].height/4 + "px";
					child.style.color = "black";
				});
				images.appendChild(image);
				lists.appendChild(child);
			});
		})
		.catch(err => console.error(err));
}



async function fetchMe() {
	try {
		const res = await fetch('/api/me', { cache: 'no-store' });
		return res.ok ? await res.json() : null;
	} catch (e) {
		return null;
	}
}

function setActiveNav(page) {
	document.querySelectorAll('.topnav a[data-page]').forEach(l => {
		l.classList.toggle('active', l.dataset.page === page);
	});
}

function showWelcome() {
	document.getElementById('signout').hidden = true;
	setActiveNav(null);
	document.getElementById('app-content').innerHTML = welcomeHtml;
}

function navigate(hash) {
	if (location.hash === hash) route();
	else location.hash = hash;
}

// #/ | #/playlists | #/sharedlist/<id> | #/<any other component>
function route() {
	if (!currentUser) { showWelcome(); return; }
	document.getElementById('signout').hidden = false;

	const m = location.hash.match(/^#\/([a-z_]*)(?:\/(\d+))?$/) || [];
	const page = m[1] || 'home';
	if (page === 'sharedlist' && m[2]) {
		setActiveNav('playlists');
		loadSharedlist(m[2]);
	} else if (page === 'playlists') {
		setActiveNav('playlists');
		loadAuthd();
	} else {
		setActiveNav(page);
		loadContent(page, 'app').then(() => {
			const greeting = document.getElementById('greeting');
			if (greeting) greeting.textContent = `hello, ${currentUser.username}`;
		});
	}
}

async function afterSignIn() {
	currentUser = await fetchMe();
	loadContent('success_modal', 'modal');
	route();
}

async function signOut() {
	await fetch('/api/signout', { method: 'POST' }).catch(() => {});
	// the spotify device belongs to the account that just left
	if (spotifyPlayer) spotifyPlayer.disconnect();
	spotifyPlayer = null;
	spotifyDeviceId = null;
	currentUser = null;
	history.replaceState(null, '', location.pathname);
	showWelcome();
}

document.addEventListener('DOMContentLoaded', async function() {
	welcomeHtml = document.getElementById('app-content').innerHTML;

	document.querySelectorAll('.topnav a[data-page]').forEach(link => {
		link.addEventListener('click', e => {
			e.preventDefault();
			navigate(`#/${link.dataset.page}`);
		});
	});
	document.getElementById('signout').addEventListener('click', signOut);
	window.addEventListener('hashchange', route);

	// the spotify callback used to land on ?id_token=true
	if (new URLSearchParams(location.search).get('id_token') === 'true') {
		history.replaceState(null, '', `${location.pathname}#/playlists`);
	}

	currentUser = await fetchMe();
	route();
});
