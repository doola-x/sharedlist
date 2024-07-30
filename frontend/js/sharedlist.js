document.addEventListener('DOMContentLoaded', function() {
    const links = document.querySelectorAll('.topnav a');

    links.forEach(link => {
        link.addEventListener('click', function(e) {
            e.preventDefault();
            links.forEach(link => link.classList.remove('active'));
            this.classList.add('active');
            const page = this.getAttribute('data-page');
            loadContent(page);
        });
    });

    function loadContent(page) {
        fetch(`components/${page}.html`)
            .then(response => {
                if (!response.ok) {
                    throw new Error('Network response was not ok');
                }
                return response.text();
            })
            .then(html => {
                document.getElementById('app-content').innerHTML = html;
                localStorage.setItem('currentPage', page);
            })
            .catch(error => {
                console.error('Error fetching the content:', error);
                document.getElementById('app-content').innerHTML = '<p>Error loading the page.</p>';
            });
    }

    const savedPage = localStorage.getItem('currentPage');
    if (savedPage) {
        loadContent(savedPage);
        // Set the active class on the corresponding link
        links.forEach(link => {
            if (link.getAttribute('data-page') === savedPage) {
                link.classList.add('active');
            }
        });
    }
});
