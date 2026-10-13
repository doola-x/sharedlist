// ambient particle field behind every page (the app is a single page).
// untouched, particles streak up and to the right and leave the screen.
// near the cursor they swarm it (attract + swirl + flock with neighbours),
// then ease back onto their streak once the cursor moves away.
(function () {
	const COUNT = 60;
	const TRAIL = 9;          // history points per tail
	const SWARM_RADIUS = 240; // cursor influence
	const NEIGHBOUR = 60;     // flocking range
	const MAX_SPEED = 6;
	const REDUCED = window.matchMedia('(prefers-reduced-motion: reduce)').matches;

	const canvas = document.createElement('canvas');
	canvas.style.cssText = 'position:fixed;inset:0;width:100%;height:100%;z-index:0;pointer-events:none;';
	document.body.prepend(canvas);
	const ctx = canvas.getContext('2d');

	let w = 0, h = 0, dpr = 1;
	const mouse = { x: -9999, y: -9999, active: false };
	const particles = [];

	const rand = (a, b) => a + Math.random() * (b - a);

	function resize() {
		dpr = window.devicePixelRatio || 1;
		w = window.innerWidth;
		h = window.innerHeight;
		canvas.width = w * dpr;
		canvas.height = h * dpr;
		ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
	}

	function spawn(anywhere) {
		const speed = REDUCED ? 0 : rand(1, 3.2);
		const angle = rand(0.08, 0.3); // slight upward angle (radians)
		const p = {
			bx: Math.cos(angle) * speed,
			by: -Math.sin(angle) * speed,
			size: rand(1.5, 5),
			alpha: rand(0.15, 0.7),
			shade: Math.floor(rand(0, 130)), // black through mid gray
			trail: []
		};
		p.vx = p.bx;
		p.vy = p.by;
		if (anywhere) {
			p.x = rand(0, w);
			p.y = rand(0, h);
		} else if (Math.random() < 0.6) { // enter from the left edge
			p.x = -rand(10, 60);
			p.y = rand(0, h * 1.1);
		} else {                           // or from the bottom edge
			p.x = rand(-60, w);
			p.y = h + rand(10, 60);
		}
		return p;
	}

	function step(p) {
		let ax = 0, ay = 0;
		let swarmed = 0; // 0..1, how strongly the cursor holds this particle

		if (mouse.active && !REDUCED) {
			const dx = mouse.x - p.x, dy = mouse.y - p.y;
			const d = Math.hypot(dx, dy);
			if (d < SWARM_RADIUS && d > 1) {
				swarmed = 1 - d / SWARM_RADIUS;
				const nx = dx / d, ny = dy / d;
				// attraction that fades out right at the cursor so they orbit, not collapse
				const pull = Math.min(d / 60, 1) * 0.22 * swarmed;
				// swirl (perpendicular) so the group circles the cursor
				const swirl = 0.14 * swarmed;
				ax += nx * pull - ny * swirl;
				ay += ny * pull + nx * swirl;
			}
		}

		if (swarmed > 0) {
			// boids-lite among other swarmed particles: align, cohere, separate
			let avx = 0, avy = 0, cx = 0, cy = 0, n = 0;
			for (const q of particles) {
				if (q === p) continue;
				const ox = q.x - p.x, oy = q.y - p.y;
				const od = Math.hypot(ox, oy);
				if (od < NEIGHBOUR && od > 0) {
					avx += q.vx; avy += q.vy;
					cx += q.x; cy += q.y;
					n++;
					if (od < 18) { // separation
						ax -= (ox / od) * 0.08;
						ay -= (oy / od) * 0.08;
					}
				}
			}
			if (n) {
				ax += (avx / n - p.vx) * 0.05 * swarmed;
				ay += (avy / n - p.vy) * 0.05 * swarmed;
				ax += (cx / n - p.x) * 0.002 * swarmed;
				ay += (cy / n - p.y) * 0.002 * swarmed;
			}
		}

		// ease back toward the streak; much weaker while swarming
		const relax = 0.012 * (1 - swarmed * 0.9);
		ax += (p.bx - p.vx) * relax;
		ay += (p.by - p.vy) * relax;

		p.vx += ax;
		p.vy += ay;
		const sp = Math.hypot(p.vx, p.vy);
		if (sp > MAX_SPEED) { p.vx *= MAX_SPEED / sp; p.vy *= MAX_SPEED / sp; }

		p.trail.unshift(p.x, p.y);
		if (p.trail.length > TRAIL * 2) p.trail.length = TRAIL * 2;
		p.x += p.vx;
		p.y += p.vy;
	}

	function offscreen(p) {
		const m = 80;
		return p.x > w + m || p.y < -m || p.x < -m * 3 || p.y > h + m * 3;
	}

	function draw(p) {
		const c = `${p.shade},${p.shade},${p.shade}`;
		ctx.lineCap = 'round';
		ctx.lineWidth = p.size * 0.8;
		// tail: fading segments back along recent positions
		let px = p.x, py = p.y;
		const segs = p.trail.length / 2;
		for (let i = 0; i < segs; i++) {
			const tx = p.trail[i * 2], ty = p.trail[i * 2 + 1];
			const f = 1 - (i + 1) / (segs + 1);
			ctx.strokeStyle = `rgba(${c},${p.alpha * f * 0.6})`;
			ctx.lineWidth = Math.max(p.size * 0.8 * f, 0.5);
			ctx.beginPath();
			ctx.moveTo(px, py);
			ctx.lineTo(tx, ty);
			ctx.stroke();
			px = tx; py = ty;
		}
		ctx.fillStyle = `rgba(${c},${p.alpha})`;
		ctx.beginPath();
		ctx.arc(p.x, p.y, p.size / 2, 0, Math.PI * 2);
		ctx.fill();
	}

	function frame() {
		requestAnimationFrame(frame);
		ctx.clearRect(0, 0, w, h);
		for (let i = 0; i < particles.length; i++) {
			const p = particles[i];
			step(p);
			if (offscreen(p)) { particles[i] = spawn(false); continue; }
			draw(p);
		}
	}

	resize();
	for (let i = 0; i < COUNT; i++) particles.push(spawn(true));
	window.addEventListener('resize', resize);
	window.addEventListener('mousemove', e => { mouse.x = e.clientX; mouse.y = e.clientY; mouse.active = true; });
	document.addEventListener('mouseleave', () => { mouse.active = false; });
	frame();
})();
