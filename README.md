# OpenGL Ray Tracing Framework

![screenshot_bunny.png](https://raw.githubusercontent.com/georgehuan1994/OpenGL-Ray-Tracing-Framework/main/screenshot/screenshot_bunny.png)

Cover Obj: [stanford bunny (4000 triangles)](https://github.com/georgehuan1994/OpenGL-Ray-Tracing-Framework/blob/main/resources/objects/bunny_4000.obj)

Cover Material: jade (baseColor: 0.55, 0.78, 0.55 / IOR = 1.79 / subsurface = 1.0)

After [《Ray Tracing in One Weekend Book Series》](https://github.com/RayTracing/raytracing.github.io) and [《LearnOpenGL》](https://github.com/JoeyDeVries/LearnOpenGL), is time to make ray-tracing in GLSL.

## Reference
### third party

| Functionality            | Library                                                      |
| ------------------------ | ------------------------------------------------------------ |
| Mesh Loading             | [assimp](https://github.com/assimp/assimp)                   |
| OpenGL Function Loader   | [glad](https://github.com/Dav1dde/glad)                      |
| Windowing and Input      | [glfw](https://github.com/glfw/glfw)                         |
| OpenGL Mathematics       | [glm](https://github.com/g-truc/glm)                         |
| Texture Loading          | [stb](https://github.com/nothings/stb)                       |
| HDR Image Reader         | [hdrloader](https://www.flipcode.com/archives/HDR_Image_Reader.shtml) |
| Graphical User Interface | [imgui](https://github.com/ocornut/imgui)                    |

### artcles

[Physically Based Shading at Disney](https://media.disneyanimation.com/uploads/production/publication_asset/48/asset/s2012_pbs_disney_brdf_notes_v3.pdf)

[Extending the Disney BRDF to a BSDF with Integrated Subsurface Scattering (Brent Burley)](https://blog.selfshadow.com/publications/s2015-shading-course/burley/s2015_pbs_disney_bsdf_notes.pdf)

[Physically Based Rendering: From Theory to Implementation](https://www.pbr-book.org/3ed-2018/contents)

### repo

[AKGWSB/EzRT: Easy Ray Tracing, a lite renderer and tutorial from theory to implement, with OpenGL](https://github.com/AKGWSB/EzRT)

[GitHub - knightcrawler25/GLSL-PathTracer: A GLSL Path Tracer](https://github.com/knightcrawler25/GLSL-PathTracer)

[blender/gpu_shader_material_principled.glsl at master · blender/blender](https://github.com/blender/blender/blob/master/source/blender/gpu/shaders/material/gpu_shader_material_principled.glsl)

## TODO

- [ ] Volume Scattering
- [ ] Compute Shader (WINDOWS only, OSX supports up to native OpenGL version 4.1)
- [ ] Texture Mapping
- [ ] Material Instances
- [ ] Scene Manager
- [ ] Gizmo

## ScreenShot

Obj: [loong (100000 triangles)](https://github.com/georgehuan1994/OpenGL-Ray-Tracing-Framework/blob/main/resources/objects/loong_100000.obj)

Material: copper (baseColor: 0.93, 0.62, 0.53 / IOR = 1.21901 / roughness = 0.2 / metallic = 1.0)

![screenshot_loong.png](https://raw.githubusercontent.com/georgehuan1994/OpenGL-Ray-Tracing-Framework/main/screenshot/screenshot_loong.png)

Obj: [panther (100000 triangles)](https://github.com/georgehuan1994/OpenGL-Ray-Tracing-Framework/blob/main/resources/objects/panther_100000.obj)

Material: brown_glass (baseColor: 1.0 / IOR = 1.45 / roughness = 0.1 / mediumType = ABSORB / mediumColor = 0.905, 0.63, 0.3 / mediumDensity = 1)

![screenshot_panther.png](https://raw.githubusercontent.com/georgehuan1994/OpenGL-Ray-Tracing-Framework/main/screenshot/screenshot_panther.png)
