// Sun & Lens Flare Component: Visible pinprick Sun and Interstellar anamorphic streaks
export function createSun(THREE, position = new THREE.Vector3(-22.0, 16.0, -18.0)) {
  const sunGroup = new THREE.Group();

  // 1. Directional Sunlight pointing toward Mars (0, 0, 0)
  const sunLight = new THREE.DirectionalLight(0xfff5ea, 3.6);
  sunLight.position.copy(position);
  sunGroup.add(sunLight);

  // 2. Visible Distant Sun Core (High-intensity unshaded white-gold disc)
  const sunMeshGeo = new THREE.SphereGeometry(0.85, 32, 32);
  const sunMeshMat = new THREE.MeshBasicMaterial({ color: 0xffffff });
  const sunMesh = new THREE.Mesh(sunMeshGeo, sunMeshMat);
  sunMesh.position.copy(position);
  sunGroup.add(sunMesh);

  // 3. Anamorphic Lens Flare Generator
  const flareGroup = new THREE.Group();
  flareGroup.position.copy(position);
  sunGroup.add(flareGroup);

  function createStreakTexture(r, g, b) {
    const cvs = document.createElement('canvas');
    cvs.width = 512;
    cvs.height = 32;
    const ctx = cvs.getContext('2d');
    const grad = ctx.createRadialGradient(256, 16, 0, 256, 16, 256);
    grad.addColorStop(0, `rgba(${r},${g},${b},1)`);
    grad.addColorStop(0.25, `rgba(${r},${g},${b},0.6)`);
    grad.addColorStop(0.65, `rgba(${r},${g},${b},0.15)`);
    grad.addColorStop(1, 'rgba(0,0,0,0)');
    ctx.fillStyle = grad;
    ctx.fillRect(0, 0, 512, 32);
    return new THREE.CanvasTexture(cvs);
  }

  // Cyan-Blue Anamorphic Streak
  const streakMat = new THREE.SpriteMaterial({
    map: createStreakTexture(130, 190, 255),
    blending: THREE.AdditiveBlending,
    transparent: true,
    opacity: 0.95
  });
  const streakSprite = new THREE.Sprite(streakMat);
  streakSprite.scale.set(38.0, 0.5, 1.0);
  flareGroup.add(streakSprite);

  // Warm Golden Core Streak
  const goldStreakMat = new THREE.SpriteMaterial({
    map: createStreakTexture(255, 215, 120),
    blending: THREE.AdditiveBlending,
    transparent: true,
    opacity: 0.85
  });
  const goldSprite = new THREE.Sprite(goldStreakMat);
  goldSprite.scale.set(22.0, 0.35, 1.0);
  flareGroup.add(goldSprite);

  return {
    group: sunGroup,
    light: sunLight,
    mesh: sunMesh,
    setFlareOpacity: (opacity) => {
      streakMat.opacity = opacity * 0.95;
      goldStreakMat.opacity = opacity * 0.85;
      sunMeshMat.opacity = opacity;
    }
  };
}
