1) I compile my code in Visual Studio 2019 in release mode. I have tested on Windows 10

2) I collaborated with Bowen Wu.
    He helped me debug normals for triangle intersections and import a model from Blender.
    I helped him figure out how to implement fog.

3) Volumetric code was based on a shader I wrote in May. Don't remember what I referenced for that

4) Issues: Jittering looks bad
    Fog doesn't work if it's not at the file's end
    Fog has constant density 
    Volumetric fog takes minutes to render, and I don't sample lights past a certain depth (20?)
    Banding in volumetric fog

5) Supersampling: run with argument "-supersample x" where x > 1
        see 09.png

    Jittering: run with "-jitter" argument
    Fog: put
        "Fog {
            density <float>
            color <color>
        }" in the scene file at the end of "Scene{.... (here)}"
        use density < .5
        
    Volumetric Fog
        run fog scene with "-shadows"
        see scenes "scene09_volumetric_fog.txt", 09.png, 00.png
        how?: sample lights at fixed steps along view ray

    Custom scenes:
        custom jackolantern (i made it; it's bad)
        see scenes 00, 08, 09 and respective pngs

    Comments:
        fun assignment!; thanks for all the boilerplate+starter code