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
			console.log(data);
			if (data.status == 'success') {
				resolve(data);
			} else if (data.status == 'error') {
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

function getUser(username) {
	const user = {
		username: username
	};
	fetch('/api/getUser', {
		method: 'POST',
		headers: {
			'Content-Type': 'application/json'
		},
		body: JSON.stringify(user)
	})
	.then(response => response.json())
	.then(user_data => {
		console.log(user_data);
		var span = document.getElementById('greeting');
		span.textContent = 'hello ' + user_data['user'] + '!';
	});
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
						loadContent('success_modal', 'modal');
						loadContent('home', 'app');
						localStorage.setItem('currentPage', 'home');
						location.reload();
					})
					.catch(err => {
						loadContent('error_modal', 'modal');
						document.getElementById('modal-content').style.display = 'block';
					});
					localStorage.setItem('currentPage', 'home');
					localStorage.setItem('username', username);
					getUser(username);
					document.getElementById('modal-content').style.display = 'block';
					// load content modal once working
				});
			}
		});	
}

document.addEventListener('DOMContentLoaded', function() {
    const links = document.querySelectorAll('.topnav a');

    links.forEach(link => {
        link.addEventListener('click', function(e) {
            e.preventDefault();
            links.forEach(link => link.classList.remove('active'));
            this.classList.add('active');
            const page = this.getAttribute('data-page');
            loadContent(page, "app");
	    if (page == 'home') {
		getUser(localStorage.getItem('username'));
	    }
        });
    });

    const savedPage = localStorage.getItem('currentPage');
    if (savedPage) {
        loadContent(savedPage, "app");
        links.forEach(link => {
            if (link.getAttribute('data-page') === savedPage) {
		if (savedPage === 'home') {
			getUser(localStorage.getItem('username'));
		}
                link.classList.add('active');
            }
        });
    }
});

