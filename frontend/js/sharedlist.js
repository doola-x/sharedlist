function loadContent(page, box) {
	fetch(`components/${page}.html`)
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
			console.log(data);
			console.log(data.items);
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
			console.log(data);
			resolve(data);
		})
		.catch(err => {
			reject(err);
		});
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
				document.getElementById('modal-content').style.display = 'block';
			})
			.catch(err => {
				loadContent('error_modal', 'modal');
				document.getElementById('modal-content').style.display = 'block';
			});
			localStorage.setItem('currentPage', 'home');
			localStorage.setItem('username', username);
			getUser(username);
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
						document.getElementById('modal-content').style.display = 'block';
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
						document.getElementById('modal-content').style.display = 'block';
					});
				});
			}
		});	
}

function loadAuthd() {
	// id token means we need to render some auth'd ui	
	loadContent('home_auth', 'app');
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
				image.addEventListener('onclick', makeSharedlist(localStorage.getItem('username'), 'spotify', 'test'));
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

function loadSharedlist(listId) {
// sharedlist needs what?
	// id from service provider
		// sharedlist pic
		// sharedlist title
		// other members?
	// internal id
	// current tracks
	// changes requested
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
					document.getElementById('modal-content').style.display = 'block';
				}
			}
			link.classList.add('active');
		    }
		});
	}
});

