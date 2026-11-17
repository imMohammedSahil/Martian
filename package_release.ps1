$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"
$stage = "release_staging\Martian_v1.0"
if (Test-Path "release_staging") {
    Remove-Item -Recurse -Force "release_staging"
}
New-Item -ItemType Directory -Path $stage -Force | Out-Null

$files = @(
    "Martian.exe",
    "freeglut.dll",
    "libwinpthread-1.dll",
    "endurance.bin",
    "ranger.bin",
    "gale_crater.bin",
    "astronaut.bin",
    "astronaut_pose1.bin",
    "astronaut_pose2.bin",
    "astronaut_pose3.bin",
    "astronaut_pose4.bin",
    "mars_surface.jpg",
    "mars_surface_4k.jpg",
    "saturn_surface_4k.jpg",
    "saturn_ring_alpha.png",
    "jupiter_surface_2k.jpg",
    "uranus_surface_2k.jpg",
    "endurance_diffuse.jpg",
    "ranger_diffuse.png",
    "martian_title.png",
    "cloud.png",
    "cloud_volume.png",
    "smoke1.png",
    "spark1.png",
    "sun_flare.png",
    "mars_sky_accurate.jpg",
    "mars_dune_sand_diffuse.jpg",
    "cabin_metal_diffuse.jpg",
    "astronaut_diffuse.png",
    "parachute_diffuse.png",
    "soundtrack.mp3",
    "mars_wind_nasa.wav",
    "timing_config.txt",
    "scene4_lighting.txt",
    "scene4_poses.txt",
    "README.md",
    "LICENSE"
)

foreach ($f in $files) {
    if (Test-Path $f) {
        Copy-Item -Path $f -Destination $stage
    } else {
        Write-Warning "Missing: $f"
    }
}

if (Test-Path "Martian_v1.0.zip") {
    Remove-Item -Force "Martian_v1.0.zip"
}
Compress-Archive -Path "$stage\*" -DestinationPath "Martian_v1.0.zip"
Remove-Item -Recurse -Force "release_staging"
Get-Item "Martian_v1.0.zip" | Select-Object Name, Length
