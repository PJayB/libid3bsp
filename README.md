# libid3bsp
Loads id Tech 3-era BSP files.

It supports:

* Quake III Arena IBSP 46
* RTCW IBSP 47
* Raven RBSP for Jedi Outcast and SoF2

It does _not_ support:

* Jedi Academy RBSP (despite those having the same header and version number)

libid3bsp's internal BSP representation is RBSP. Older IBSPs will be upgraded to
RBSP.

## Usage

* Load the BSP file into memory
* Call `BSP::Create`

```
BSP* bsp = BSP::Create(&bspData[0], bspData.size());
```

* (Optional) Tessellate the patches into triangle meshes

```
for (auto& f : bsp.Faces)
{
    if (f.FaceType != BSP::kPatch)
        continue;

    BSP::Tessellate(f, bsp.Vertices, bsp.Indices, numSubdivions);
}
```

**NOTE**: rendering the BSP requires you to check surface flags and lightmapping
flags, which is left as an exercise for the reader. Common ones to look for are:

* `BSP::kSurfNoDraw` in `BSP::Texture::TextureFlags`. Don't draw these!
* Handle `BSP::eLightMapIndices` in `BSP::Face::LightMapIDs`. Depending on the
  value, you will want to use a lightmap texture or rely on vertex coloring.

You can find an example at my [bsp2obj project](https://github.com/PJayB/bsp2obj).

## To-Do

* Documentation in the header
* Better usage documentation
* Code cleanup, formatting
* Include other common surface and texture flags
* Load .shader files and parse them for textures
* Load light grid data for RBSPs
