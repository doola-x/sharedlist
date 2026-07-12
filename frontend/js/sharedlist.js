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

function loadContent(page, box) {
	activeTimers.forEach(clearTimeout);
	activeTimers.length = 0;
	return fetch(`components/${page}.html`)
	    .then(response => {
		if (!response.ok) {
		    localStorage.removeItem('currentPage');
		    throw new Error('Network response was not ok');
		}
		return response.text();
	    })
	    .then(html => {
		if (box == "app") {
			document.getElementById('app-content').innerHTML = html;
			localStorage.setItem('currentPage', page);
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

function fetchPlaylists(username) {
	return new Promise((resolve, reject) => {
		const user = {
			username: username
		};
		fetch('/api/spotify_playlists', {
			method: 'POST',
			headers: {
				'Content-Type': 'application/json'
			},
			body: JSON.stringify(user)
		})
		.then(response => response.json())
		.then(data => {
			resolve(data);
		})
		.catch(err => {
			reject(err);
		});
	});
}

function makeSharedlist(username, type, id) {
	return new Promise((resolve, reject) => {
		const body = {
			username: username,
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
		.then(response => response.json())
		.then(data => {
			loadSharedlist(data.sharedlist_id);
			resolve(data);
		})
		.catch(err => {
			reject(err);
		});
	});
}

function loadSharedlist(sharedlistId) {
	loadContent('home_sharedlist', 'app').then(async () => {
		const tbody = document.querySelector('.tracklist tbody');
		const limit = 20;
		localStorage.setItem('rowIndex', 1);
		localStorage.setItem('offset', 0);
		localStorage.setItem('loading', false);
		tbody.innerHTML = '';

		async function fetchBatch(offset) {
			let rowIndex = localStorage.getItem('rowIndex');
			localStorage.setItem('loading', true);
			try {
				const res = await fetch(`/api/sharedlist?sharedlist_id=${sharedlistId}&offset=${offset}&limit=${limit}`);
				const data = await res.json();
				console.log(data);
				if (data["items"] == null || data["items"].length == 0) {
					localStorage.setItem('loading', false);
					return false;
				}
				let tracks = data["items"];
				tracks.forEach(track => {
					const tr = document.createElement('tr');
					tr.className = 'tracklist_item';
					tr.innerHTML = `<th scope="row">${rowIndex++}</th>` +
						`<td class="song_select" style="max-width: 20px;"><input type="checkbox"></td>` +
						`<td>${track.name}</td>` +
						`<td>${track.artists}</td>` +
						`<td>${track.album}</td>` +
						`<td class="spotify_id" style="display: none;">${track.spotify_id}</td>`;
					tbody.appendChild(tr);
				});
				localStorage.setItem('offset', offset + tracks.length);
				localStorage.setItem('loading', false);
				localStorage.setItem('has_next', data["has_next"]);
			} catch(err) {
				console.error('Error fetching tracks:', err)
				localStorage.setItem('loading', false);
			}

			return true;
		}

		const tcontainer = document.querySelector('.tracklist-scroller');
		const handleScroll = throttle(async () => {
			let next = localStorage.getItem('has_next')
			let loading = localStorage.getItem('loading');
			let offset = parseInt(localStorage.getItem('offset'));
			console.log('even listener fired, hn: ' + next + ', loading: ' + loading
			 + ', offset: ' + offset);
			if (loading == "true") {
				return;
			}
			if (next == "false") {
				tcontainer.removeEventListener('scroll', handleScroll); 
			}
			const scrollTop = tcontainer.scrollTop;     
			const scrollHeight = tcontainer.scrollHeight; 
			const clientHeight = tcontainer.clientHeight; 
			let delta = scrollHeight - scrollTop - clientHeight 
			console.log('delta: ' + delta);
			if (delta <= 200) {
				await fetchBatch(offset);
			}
		}, 300);
		tcontainer.addEventListener('scroll', handleScroll); 
		let result = await fetchBatch(0);
		console.log('batch returned result ' + result);
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
				console.log('new sleep ms ' + sleep_ms);
			}
		}
	});
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
	return new Promise((resolve, reject) => {
		const user = {
			username: username,
			password: password
		};
		fetch('/api/signup', {
			method: 'POST',
			headers: {
				'Content-Type': 'application/json'
			},
			body: JSON.stringify(user)
		})
		.then(response => response.json())
		.then(data => {
			signIn(username, password)
			.then(data => {
				loadContent('success_modal', 'modal');
				loadContent('home', 'app');
				localStorage.setItem('currentPage', 'home');
			})
			.catch(err => {
				loadContent('error_modal', 'modal');
			});
			localStorage.setItem('currentPage', 'home');
			localStorage.setItem('username', username);
		});
	});
}

function hideModal() {
	document.getElementById('modal-content').style.display = 'none';
}

function spotifyAuthLaunch() {
	window.location.href = "/api/spotify_signin?user=" + localStorage.getItem('username');
}

function spawnSignUpIn(page) {
	fetch(`/components/${page}.html`)
		.then(response => {
			if (!response.ok) {
				localStorage.removeItem('currentPage');
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
					.then(data => {
						// do something?
						location.reload();
					})
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
					.then(data => {
						if (data['status'] == 'success') {
							loadContent('success_modal', 'modal');
							loadContent('home', 'app');
							localStorage.setItem('currentPage', 'home');
							localStorage.setItem('fromSignin', 'y');
							localStorage.setItem('username', username);
							location.reload();
						} else {
							loadcontent('error_modal', 'modal');
							document.getElementById('modal-content').style.display = 'block';
						}
					})
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
	fetchPlaylists(localStorage.getItem('username'))
		.then(data => {
			console.log(data);
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
					console.log('img listener row.id,' + row.id);
					makeSharedlist(localStorage.getItem('username'), 'spotify', row.id) 
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



document.addEventListener('DOMContentLoaded', function() {
	const links = document.querySelectorAll('.topnav a');
	const urlParams = new URLSearchParams(window.location.search);
	const idToken = urlParams.get("id_token");
	links.forEach(link => {
		link.addEventListener('click', function(e) {
			e.preventDefault();
			links.forEach(link => link.classList.remove('active'));
			this.classList.add('active');
			const page = this.getAttribute('data-page');
			loadContent(page, "app");
			if (page === 'home' && idToken) {
				loadAuthd();
			}
		});
	});

	let savedPage = localStorage.getItem('currentPage');
	if (savedPage) {
		if (savedPage === 'home_auth') savedPage = 'home';
		loadContent(savedPage, "app");
		links.forEach(link => {
		    if (link.getAttribute('data-page') === savedPage) {
			if (savedPage === 'home' || savedPage === 'home_auth') {
				if (idToken === "true"){
					loadAuthd();
				}
				if (localStorage.getItem('fromSignin') == 'y') {
					localStorage.setItem('fromSignin', 'n');
					loadContent('success_modal', 'modal');
				}
			}
			link.classList.add('active');
		    }
		});
	}
});

