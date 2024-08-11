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
			document.getelementById('modal-content').innerHTML = html;
		}
	    })
	    .catch(error => {
		console.error('Error fetching the content:', error);
		document.getElementById('app-content').innerHTML = '<p>Error loading the page.</p>';
	    });
}

function getUser() {
	fetch('/api/getUser')
	.then(response => response.json())
	.then(user_data => {
		console.log(user_data);
		var span = document.getElementById('greeting');
		span.textContent = 'hello ' + user_data['user'] + '!';
	});
}

function spawnSignup() {
	fetch('/components/signup.html')
		.then(response => {
			if (!response.ok) {
				localStorage.removeItem('currentPage');
				throw new Error('Network response was not ok');
			}
			return response.text();
		})
		.then(html => {
			document.getElementById('app-content').innerHTML = html;
			document.getElementById('signupform').addEventListener('submit', function(e) {
				var username = document.getElementById('username').value;
				var password = document.getElementById('password').value;
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
					console.log(data);
				});
				localStorage.setItem('currentPage', 'home');
				loadContent("success_modal", "modal");
			});
			localStorage.setItem('currentPage', 'signup');
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
		console.log('page is home');
		getUser();
	    }
        });
    });

    const savedPage = localStorage.getItem('currentPage');
    if (savedPage) {
        loadContent(savedPage, "app");
        links.forEach(link => {
            if (link.getAttribute('data-page') === savedPage) {
		if (savedPage === 'home') {
			getUser();
		}
                link.classList.add('active');
            }
        });
    }
});

