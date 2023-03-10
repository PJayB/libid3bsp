#include <id3bsp/BSP.h>
#include <cassert>
#include <memory.h>

namespace id3bsp
{

namespace IBSP
{
	struct BrushSide
	{
		uint32_t				Plane;
		int32_t					TextureIndex;
	};
	
	struct Vertex
	{
		BSP::Vec3				Position;
		BSP::Vec2				TexCoord;
		BSP::Vec2				LMCoord;	// Lightmap coordinate.
		BSP::Vec3				Normal;
		BSP::Color4				Color;
	};
	
	struct Face
	{
		int32_t					TextureID;
		int32_t					FogID;
		uint32_t				FaceType;

		uint32_t				StartVertexIndex;
		uint32_t				NumVertices;

		uint32_t				StartIndex;
		uint32_t				NumIndices;

		uint32_t				LightMapID;
		uint32_t				LMX; // "The face's lightmap corner in the image"
		uint32_t				LMY;
		uint32_t				LMWidth; // Size of the lightmap section
		uint32_t				LMHeight;
		
		BSP::Vec3				LMOrigin; // 3D origin of lightmap (???)
		std::array<BSP::Vec3,3>	LMVecs; // 3D space for s and t unit vectors (???)
		
		BSP::Bounds2D			BezierDimensions;
	};
	
	struct LightVolume
	{
		BSP::Color3				Ambient;
		BSP::Color3				Directional;
		std::array<uint8_t, 2>	Direction; // phi, theta.
	};
}

template<typename T, size_t N> void copy_array(std::array<T, N>& dst, const std::array<T, N>& src)
{
    memcpy(dst.data(), src.data(), sizeof(T) * src.size());
}

template<typename Array> void add_vector(Array& r, const Array& a, const Array& b)
{
	for (int i = 0; i < r.size(); ++i)
		r[i] = a[i] + b[i];
}

template<typename Array> void scale_vector(Array& r, const Array& a, float b)
{
	for (int i = 0; i < r.size(); ++i)
		r[i] = a[i] * b;
}

static BSP::BrushSide ibsp_to_rbsp(const IBSP::BrushSide& b)
{
    BSP::BrushSide a;
    memset(&a, 0, sizeof(a));

    a.Plane = b.Plane;
    a.TextureIndex = b.TextureIndex;
    a.DrawSurfIndex = -1;
    return a;
}

static BSP::Vertex ibsp_to_rbsp(const IBSP::Vertex& b)
{
    BSP::Vertex a;
    memset(&a, 0, sizeof(a));

    copy_array(a.Position, b.Position);
    copy_array(a.TexCoord, b.TexCoord);
    copy_array(a.LMCoord[0], b.LMCoord);
    copy_array(a.Normal, b.Normal);
    return a;
}

static BSP::Face ibsp_to_rbsp(const IBSP::Face& b)
{
    BSP::Face a;
    memset(&a, 0, sizeof(a));

    a.TextureID = b.TextureID;
    a.FogID = b.FogID;
    a.FaceType = b.FaceType;
    a.StartVertexIndex = b.StartVertexIndex;
    a.NumVertices = b.NumVertices;
    a.StartIndex = b.StartIndex;
    a.NumIndices = b.NumIndices;
    a.LightMapIDs[0] = b.LightMapID;
    for (int i = 1; i < BSP::kMaxLightMaps; ++i)
        a.LightMapIDs[i] = BSP::kLightMapNone;
    a.LMX[0] = b.LMX;
    a.LMY[0] = b.LMY;
	a.LMWidth = b.LMWidth;
	a.LMHeight = b.LMHeight;
    copy_array(a.LMOrigin, b.LMOrigin);
    copy_array(a.LMVecs, b.LMVecs);
    copy_array(a.BezierDimensions, b.BezierDimensions);
    return a;
}

static BSP::LightVolume ibsp_to_rbsp(const IBSP::LightVolume& b)
{
    BSP::LightVolume a;
    memset(&a, 0, sizeof(a));

    copy_array(a.Ambient[0], b.Ambient);
    copy_array(a.Directional[0], b.Directional);
    copy_array(a.Direction, b.Direction);
    return a;
}

BSP* BSP::Create( const void* lpData, size_t cbSize )
{
	BSP* bsp = new BSP;
	
	if ( !bsp->Load( (const uint8_t*) lpData, cbSize ) )
	{
		delete bsp;
		return NULL;
	}
	
	return bsp;
}

BSP::BSP()
{
}

BSP::~BSP()
{
	Unload();
}
	
template<class T>
static void ReadLump( const uint8_t* cursor, std::vector<T>& array, const BSP::Lump& lump )
{
	if ( lump.Length > 0 )
	{
		array.resize( lump.Length / sizeof( T ) );

		assert( array.size() * sizeof( T ) == lump.Length );

		memcpy( 
			&array[0], 
			&cursor[lump.Offset], 
			array.size() * sizeof( T ));
	}
}

template<class IBSP_T, class RBSP_T>
static void UpgradeLump(const uint8_t* cursor, std::vector<RBSP_T>& rbspArray, const BSP::Lump& lump)
{
	std::vector<IBSP_T> ibspArray;
	ReadLump(cursor, ibspArray, lump);

	// Convert from IBSP to RBSP
	rbspArray.reserve(ibspArray.size());
	for (const auto& i : ibspArray) {
		rbspArray.push_back(ibsp_to_rbsp(i));
	}
}

static void ReadLump( const uint8_t* cursor, std::string& str, const BSP::Lump& lump )
{
	if ( lump.Length > 0 )
	{
		str.reserve( lump.Length + 1 );
		str = (const char*) &cursor[lump.Offset];
	}
}

bool BSP::Load( const uint8_t* lpBytes, size_t cbSize )
{
	Unload();
		
	// Read the header
	const Header* header = (const Header*) lpBytes;

	Format = header->Format;
	
	// Read the lumps
	const Lump* lumps = (const Lump*) (lpBytes + sizeof(Header));
	
	// Read basic lumps
	// - entities (done later)
	ReadLump( lpBytes, Materials,			lumps[kTextures] );
	ReadLump( lpBytes, Planes,				lumps[kPlanes] );
	ReadLump( lpBytes, Nodes,				lumps[kNodes] );
	ReadLump( lpBytes, Leaves,				lumps[kLeaves] );
	ReadLump( lpBytes, LeafFaces,			lumps[kLeafFaces] );
	ReadLump( lpBytes, LeafBrushes,			lumps[kLeafBrushes] );
	ReadLump( lpBytes, Models,				lumps[kModels] );
	ReadLump( lpBytes, Brushes,				lumps[kBrushes] );
	if (header->Format == kRBSPFormat) {
		ReadLump(lpBytes, BrushSides,		lumps[kBrushSides]);
		ReadLump(lpBytes, Vertices,			lumps[kVertices]);
		ReadLump(lpBytes, Faces,			lumps[kFaces]);
		ReadLump(lpBytes, LightVolumes, 	lumps[kLightVolumes]);
	} else {
		UpgradeLump<IBSP::BrushSide>
				(lpBytes, BrushSides, 		lumps[kBrushSides]);
		UpgradeLump<IBSP::Vertex>
				(lpBytes, Vertices,			lumps[kVertices]);
		UpgradeLump<IBSP::Face>
				(lpBytes, Faces,			lumps[kFaces]);
		UpgradeLump<IBSP::LightVolume>
				(lpBytes, LightVolumes,		lumps[kLightVolumes]);
	}
	ReadLump( lpBytes, Indices,				lumps[kIndices] );
	ReadLump( lpBytes, Fogs,				lumps[kFogs] );
	ReadLump( lpBytes, LightMaps,			lumps[kLightmaps] );
	// - vis data (done later)

	// - todo: light grid data
	
	// Read the entities info
	if ( lumps[kEntities].Length )
	{
		ReadLump( lpBytes, EntityString, lumps[kEntities] );
	}

	// Visibility data.
	NumClusters = 0;
	ClusterVisDataSize = 0;
	if ( lumps[kVisData].Length )
	{
		const uint32_t* visInfo = (const uint32_t*) (lpBytes + lumps[kVisData].Offset);
		NumClusters = visInfo[0];
		ClusterVisDataSize = visInfo[1];
		
		ClusterBits.resize( NumClusters * ClusterVisDataSize );
		memcpy( &ClusterBits[0], &visInfo[2], NumClusters * ClusterVisDataSize );
	}
	
	// Done and DONE.

	return true;
}

void BSP::Unload()
{
	Materials.clear();
	Planes.clear();
	Nodes.clear();
	Leaves.clear();
	LeafFaces.clear();
	LeafBrushes.clear();
	Models.clear();
	Brushes.clear();
	BrushSides.clear();
	Vertices.clear();
	Indices.clear();
	Fogs.clear();
	Faces.clear();
	LightMaps.clear();
	LightVolumes.clear();
	EntityString.clear();
	ClusterBits.clear();
	
	NumClusters = 0;
	ClusterVisDataSize = 0;
	Format = 0;
}

void BSP::Tessellate(
	BSP::Face& f,
	BSP::TVertexList& vertices,
	BSP::TIndexList& indices,
	int numSubdivions)
{
	Tessellate(f, vertices, vertices, indices, numSubdivions);
}

void BSP::Tessellate(
	BSP::Face& f,
	const BSP::TVertexList& ogVertices,
	BSP::TVertexList& newVertices,
	BSP::TIndexList& newIndices,
	int numSubdivions)
{
	int verticesPerRow = f.BezierDimensions[0];
	int numPatchesX = f.BezierDimensions[0] / 2;
	int numPatchesY = f.BezierDimensions[1] / 2;

	auto cpBaseOffset = f.StartVertexIndex;

	f.StartVertexIndex = static_cast<int>(newVertices.size());
	f.StartIndex = static_cast<int>(newIndices.size());

	int indexOffset = 0;
	for (int j = 0; j < numPatchesY; ++j) {
		for (int i = 0; i < numPatchesX; ++i) {
			int cpPatchOffs1 = cpBaseOffset + j * verticesPerRow * 2 + i * 2;
			int cpPatchOffs2 = cpPatchOffs1 + verticesPerRow;
			int cpPatchOffs3 = cpPatchOffs2 + verticesPerRow;

			const BSP::Vertex* controlPoints[3] = {
				&ogVertices[cpPatchOffs1],
				&ogVertices[cpPatchOffs2],
				&ogVertices[cpPatchOffs3]
			};

			indexOffset += Tessellate(
				controlPoints,
				newVertices,
				newIndices,
				numSubdivions,
				indexOffset);
		}
	}

	f.NumIndices = static_cast<int>(newIndices.size()) - f.StartIndex;
	f.NumVertices = static_cast<int>(newVertices.size()) - f.StartVertexIndex;
	f.FaceType = BSP::kPolygon;
}

// Shamelessly stolen from git@github.com:leezh/bspviewer.git
int BSP::Tessellate(
	const BSP::Vertex** controls,
	BSP::TVertexList& vertices,
	BSP::TIndexList& indices,
	int numSubdivisions,
	int indexOffset)
{
	int vOffset = static_cast<int>(vertices.size());
	int iOffset = static_cast<int>(indices.size());

    int L1 = numSubdivisions + 1;

    for (int j = 0; j <= numSubdivisions; ++j)
    {
        float a = (float)j / numSubdivisions;
        float b = 1.f - a;

        vertices.push_back(controls[0][0] * b * b + controls[1][0] * 2 * b * a + controls[2][0] * a * a);
    }

    for (int i = 1; i <= numSubdivisions; ++i)
    {
        float a = (float)i / numSubdivisions;
        float b = 1.f - a;

        BSP::Vertex temp[3];

        for (int j = 0; j < 3; ++j)
        {
            temp[j] = controls[j][0] * b * b + controls[j][1] * 2 * b * a + controls[j][2] * a* a;
        }

        for (int j = 0; j <= numSubdivisions; ++j)
        {
            float a = (float)j / numSubdivisions;
            float b = 1.f - a;

			assert(vertices.size() == vOffset + i * L1 + j);
            vertices.push_back(temp[0] * b * b + temp[1] * 2 * b * a + temp[2] * a * a);
        }
    }

    for (int i = 0; i < numSubdivisions; ++i)
    {
        for (int j = 0; j < numSubdivisions; ++j)
        {
            int offset = (i * numSubdivisions + j) * 6;
			assert(indices.size() == iOffset + offset);

            indices.push_back((i    ) * L1 + (j    ) + indexOffset);
            indices.push_back((i    ) * L1 + (j + 1) + indexOffset);
            indices.push_back((i + 1) * L1 + (j + 1) + indexOffset);

            indices.push_back((i + 1) * L1 + (j + 1) + indexOffset);
            indices.push_back((i + 1) * L1 + (j    ) + indexOffset);
            indices.push_back((i    ) * L1 + (j    ) + indexOffset);
        }
    }

	return static_cast<int>(vertices.size()) - vOffset;
}

}

id3bsp::BSP::Vertex operator + (const id3bsp::BSP::Vertex& v1,
	const id3bsp::BSP::Vertex& v2)
{
	using namespace id3bsp;
    BSP::Vertex temp;
    add_vector(temp.Position, v1.Position, v2.Position);
	add_vector(temp.TexCoord, v1.TexCoord, v2.TexCoord);
    for (int i = 0; i < BSP::kMaxLightMaps; ++i) {
        add_vector(temp.LMCoord[i], v1.LMCoord[i], v2.LMCoord[i]);
    }
    add_vector(temp.Normal, v1.Normal, v2.Normal);
    return temp;
}

id3bsp::BSP::Vertex operator * (const id3bsp::BSP::Vertex& v1, const float& d)
{
	using namespace id3bsp;
    BSP::Vertex temp;
    scale_vector(temp.Position, v1.Position, d);
    scale_vector(temp.TexCoord, v1.TexCoord, d);
    for (int i = 0; i < BSP::kMaxLightMaps; ++i) {
        scale_vector(temp.LMCoord[i], v1.LMCoord[i], d);
    }
    scale_vector(temp.Normal, v1.Normal, d);
    return temp;
}