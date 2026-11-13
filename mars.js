// Mars Component: Clean photorealistic Mars with direct surface shading (zero outline artifacts)
export function createMars(THREE, radius = 10.0) {
  const marsGroup = new THREE.Group();

  // Surface Sphere
  const marsGeo = new THREE.SphereGeometry(radius, 128, 128);
  const textureLoader = new THREE.TextureLoader();

  const marsTexture = textureLoader.load('mars_surface.jpg');
  marsTexture.generateMipmaps = true;
  marsTexture.minFilter = THREE.LinearMipmapLinearFilter;

  const marsMat = new THREE.MeshStandardMaterial({
    map: marsTexture,
    roughness: 0.70,
    metalness: 0.02
  });

  const marsMesh = new THREE.Mesh(marsGeo, marsMat);
  marsMesh.rotation.y = 2.1; // Face radiant, sunlit Tharsis & Olympus Mons toward camera
  marsGroup.add(marsMesh);

  return {
    group: marsGroup,
    mesh: marsMesh,
    update: (delta) => {
      marsMesh.rotation.y += 0.0006;
    }
  };
}
