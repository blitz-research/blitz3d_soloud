
#ifndef BBBLITZ3D_H
#define BBBLITZ3D_H

#include "bbsys.h"
#include "../gxruntime/gxscene.h"

// Forward declarations
class gxScene;
class gxFileSystem;
class World;
class Brush;
class Texture;
class Entity;
class Listener;
class MeshModel;
class Model;
class Camera;
class Light;
class Sprite;
class Terrain;
class Object;
class Surface;
class Line;
class Vector;
class Transform;
class Collision;
class CachedTexture;

// External variables
extern gxScene *gx_scene;
extern gxFileSystem *gx_filesys;
extern float stats3d[10];

// Function declarations
static Entity* loadEntity( string t, int hint );
static void collapseMesh( MeshModel *mesh, Entity *e );
static void insert( Entity *e );
static Entity* insertEntity( Entity *e, Entity *p );
static void erase( Entity *e );
static Entity* findChild( Entity *e, const string &t );

///////////////////////////
// GLOBAL WORLD COMMANDS //
///////////////////////////
void bbLoaderMatrix( BBStr *ext, float xx, float xy, float xz, float yx, float yy, float yz, float zx, float zy, float zz );
int bbHWTexUnits();
int bbGfxDriverCaps3D();
void bbHWMultiTex( int enable );
void bbWBuffer( int enable );
void bbDither( int enable );
void bbAntiAlias( int enable );
void bbWireFrame( int enable );
void bbAmbientLight( float r, float g, float b );
void bbClearCollisions();
void bbCollisions( int src_type, int dest_type, int method, int response );

void bbUpdateWorld( float elapsed );
void bbCaptureWorld();
void bbRenderWorld( float tween );
int bbTrisRendered();
float bbStats3D( int n );

//////////////////////
// TEXTURE COMMANDS //
//////////////////////
Texture* bbLoadTexture( BBStr *file, int flags );
Texture* bbLoadAnimTexture( BBStr *file, int flags, int w, int h, int first, int cnt );
Texture* bbCreateTexture( int w, int h, int flags, int frames );
void bbFreeTexture( Texture *t );
void bbTextureBlend( Texture *t, int blend );
void bbTextureCoords( Texture *t, int flags );
void bbScaleTexture( Texture *t, float u_scale, float v_scale );
void bbRotateTexture( Texture *t, float angle );
void bbPositionTexture( Texture *t, float u_pos, float v_pos );
int bbTextureWidth( Texture *t );
int bbTextureHeight( Texture *t );
BBStr* bbTextureName( Texture *t );
void bbSetCubeFace( Texture *t, int face );
void bbSetCubeMode( Texture *t, int mode );
gxCanvas* bbTextureBuffer( Texture *t, int frame );
void bbClearTextureFilters();
void bbTextureFilter( BBStr *t, int flags );

////////////////////
// BRUSH COMMANDS //
////////////////////
Brush* bbCreateBrush( float r, float g, float b );
Brush* bbLoadBrush( BBStr *file, int flags, float u_scale, float v_scale );
void bbFreeBrush( Brush *b );
void bbBrushColor( Brush *br, float r, float g, float b );
void bbBrushAlpha( Brush *b, float alpha );
void bbBrushShininess( Brush *b, float n );
void bbBrushTexture( Brush *b, Texture *t, int frame, int index );
Texture* bbGetBrushTexture( Brush *b, int index );
void bbBrushBlend( Brush *b, int blend );
void bbBrushFX( Brush *b, int fx );

///////////////////
// MESH COMMANDS //
///////////////////
Entity* bbCreateMesh( Entity *p );
Entity* bbLoadMesh( BBStr *f, Entity *p );
Entity* bbLoadAnimMesh( BBStr *f, Entity *p );
Entity* bbCreateCube( Entity *p );
Entity* bbCreateSphere( int segs, Entity *p );
Entity* bbCreateCylinder( int segs, int solid, Entity *p );
Entity* bbCreateCone( int segs, int solid, Entity *p );
Entity* bbCopyMesh( MeshModel *m, Entity *p );
void bbScaleMesh( MeshModel *m, float x, float y, float z );
void bbRotateMesh( MeshModel *m, float x, float y, float z );
void bbPositionMesh( MeshModel *m, float x, float y, float z );
void bbFitMesh( MeshModel *m, float x, float y, float z, float w, float h, float d, int uniform );
void bbFlipMesh( MeshModel *m );
void bbPaintMesh( MeshModel *m, Brush *b );
void bbAddMesh( MeshModel *src, MeshModel *dest );
void bbUpdateNormals( MeshModel *m );
void bbLightMesh( MeshModel *m, float r, float g, float b, float range, float x, float y, float z );
float bbMeshWidth( MeshModel *m );
float bbMeshHeight( MeshModel *m );
float bbMeshDepth( MeshModel *m );
int bbMeshesIntersect( MeshModel *a, MeshModel *b );
int bbCountSurfaces( MeshModel *m );
Surface* bbGetSurface( MeshModel *m, int index );
void bbMeshCullBox( MeshModel *m, float x, float y, float z, float width, float height, float depth );


//////////////////////
// SURFACE COMMANDS //
//////////////////////
Surface* bbFindSurface( MeshModel *m, Brush *b );
Surface* bbCreateSurface( MeshModel *m, Brush *b );
Brush* bbGetSurfaceBrush( Surface *s );
Brush* bbGetEntityBrush( Model *m );
void bbClearSurface( Surface *s, int verts, int tris );
void bbPaintSurface( Surface *s, Brush *b );
int bbAddVertex( Surface *s, float x, float y, float z, float tu, float tv, float tw );
int bbAddTriangle( Surface *s, int v0, int v1, int v2 );
void bbVertexCoords( Surface *s, int n, float x, float y, float z );
void bbVertexNormal( Surface *s, int n, float x, float y, float z );
void bbVertexColor( Surface *s, int n, float r, float g, float b, float a );
void bbVertexTexCoords( Surface *s, int n, float u, float v, float w, int set );
int bbCountVertices( Surface *s );
int bbCountTriangles( Surface *s );
float bbVertexX( Surface *s, int n );
float bbVertexY( Surface *s, int n );
float bbVertexZ( Surface *s, int n );
float bbVertexNX( Surface *s, int n );
float bbVertexNY( Surface *s, int n );
float bbVertexNZ( Surface *s, int n );
float bbVertexRed( Surface *s, int n );
float bbVertexGreen( Surface *s, int n );
float bbVertexBlue( Surface *s, int n );
float bbVertexAlpha( Surface *s, int n );
float bbVertexU( Surface *s, int n, int t );
float bbVertexV( Surface *s, int n, int t );
float bbVertexW( Surface *s, int n, int t );
int bbTriangleVertex( Surface *s, int n, int v );

/////////////////////
// CAMERA COMMANDS //
/////////////////////
Entity * bbCreateCamera( Entity *p );
void bbCameraZoom( Camera *c, float zoom );
void bbCameraRange( Camera *c, float nr, float fr );
void bbCameraClsColor( Camera *c, float r, float g, float b );
void bbCameraClsMode( Camera *c, int cls_color, int cls_zbuffer );
void bbCameraProjMode( Camera *c, int mode );
void bbCameraViewport( Camera *c, int x, int y, int w, int h );
void bbCameraFogRange( Camera *c, float nr, float fr );
void bbCameraFogColor( Camera *c, float r, float g, float b );
void bbCameraFogMode( Camera *c, int mode );
int bbCameraProject( Camera *c, float x, float y, float z );
float bbProjectedX();
float bbProjectedY();
float bbProjectedZ();
static Object *doPick( const Line &l, float radius );
Entity* bbCameraPick( Camera *c, float x, float y );
Entity* bbLinePick( float x, float y, float z, float dx, float dy, float dz, float radius );
Entity* bbEntityPick( Object *src, float range );
int bbEntityVisible( Object *src, Object *dest );
int bbEntityInView( Entity *e, Camera *c );
float bbPickedX();
float bbPickedY();
float bbPickedZ();
float bbPickedNX();
float bbPickedNY();
float bbPickedNZ();
float bbPickedTime();
Object* bbPickedEntity();
void* bbPickedSurface();
int bbPickedTriangle();

////////////////////
// LIGHT COMMANDS //
////////////////////
Entity* bbCreateLight( int type, Entity *p );
void bbLightColor( Light *light, float r, float g, float b );
void bbLightRange( Light *light, float range );
void bbLightConeAngles( Light *light, float inner, float outer );

////////////////////
// PIVOT COMMANDS //
////////////////////
Entity* bbCreatePivot( Entity *p );

/////////////////////
// SPRITE COMMANDS //
/////////////////////
Entity* bbCreateSprite( Entity *p );
Entity* bbLoadSprite( BBStr *file, int flags, Entity *p );
void bbRotateSprite( Sprite *s, float angle );
void bbScaleSprite( Sprite *s, float x, float y );
void bbHandleSprite( Sprite *s, float x, float y );
void bbSpriteViewMode( Sprite *s, int mode );

/////////////////////
// MIRROR COMMANDS //
/////////////////////
Entity* bbCreateMirror( Entity *p );

////////////////////
// PLANE COMMANDS //
////////////////////
Entity* bbCreatePlane( int segs, Entity *p );

//////////////////
// MD2 COMMANDS //
//////////////////
Entity* bbLoadMD2( BBStr *file, Entity *p );
void bbAnimateMD2( MD2Model *m, int mode, float speed, int first, int last, float trans );
float bbMD2AnimTime( MD2Model *m );
int bbMD2AnimLength( MD2Model *m );
int bbMD2Animating( MD2Model *m );

//////////////////
// BSP Commands //
//////////////////
Entity* bbLoadBSP( BBStr *file, float gam, Entity *p );
void bbBSPAmbientLight( Q3BSPModel *t, float r, float g, float b );
void bbBSPLighting( Q3BSPModel *t, int lmap );

//////////////////////
// TERRAIN COMMANDS //
//////////////////////
static float terrainHeight( Terrain *t, float x, float z );
static Vector terrainVector( Terrain *t, float x, float y, float z );
Entity* bbCreateTerrain( int n, Entity *p );
Entity* bbLoadTerrain( BBStr *file, Entity *p );
void bbTerrainDetail( Terrain *t, int n, int m );
void bbTerrainShading( Terrain *t, int enable );
float bbTerrainX( Terrain *t, float x, float y, float z );
float bbTerrainY( Terrain *t, float x, float y, float z );
float bbTerrainZ( Terrain *t, float x, float y, float z );
int bbTerrainSize( Terrain *t );
float bbTerrainHeight( Terrain *t, int x, int z );
void bbModifyTerrain( Terrain *t, int x, int z, float h, int realtime );

////////////////////
// AUDIO COMMANDS //
////////////////////
Entity* bbCreateListener( Entity *p, float roll, float dopp, float dist );

#if BB_FMOD_ENABLED
gxChannel* bbEmitSound( gxSound *sound, Object *o );
#else
uint32_t bbEmitSound( Sound *sound, Object *o );
#endif

/////////////////////
// ENTITY COMMANDS //
/////////////////////
Entity* bbCopyEntity( Entity *e, Entity *p );
void bbFreeEntity( Entity *e );
void bbHideEntity( Entity *e );
void bbShowEntity( Entity *e );
void bbEntityParent( Entity *e, Entity *p, int global );
int bbCountChildren( Entity *e );
Entity* bbGetChild( Entity *e, int index );
Entity* bbFindChild( Entity *e, BBStr *t );

////////////////////////
// ANIMATION COMMANDS //
////////////////////////
int bbLoadAnimSeq( Object *o, BBStr *f );
void bbSetAnimTime( Object *o, float time, int seq );
void bbAnimate( Object *o, int mode, float speed, int seq, float trans );
void bbSetAnimKey( Object *o, int frame, int pos_key, int rot_key, int scl_key );
int bbExtractAnimSeq( Object *o, int first, int last, int seq );
int bbAddAnimSeq( Object *o, int length );
int bbAnimSeq( Object *o );
float bbAnimTime( Object *o );
int bbAnimLength( Object *o );
int bbAnimating( Object *o );

////////////////////////////////
// ENTITY SPECIAL FX COMMANDS //
////////////////////////////////
void bbPaintEntity( Model *m, Brush *b );
void bbEntityColor( Model *m, float r, float g, float b );
void bbEntityAlpha( Model *m, float alpha );
void bbEntityShininess( Model *m, float shininess );
void bbEntityTexture( Model *m, Texture *t, int frame, int index );
void bbEntityBlend( Model *m, int blend );
void bbEntityFX( Model *m, int fx );
void bbEntityAutoFade( Model *m, float nr, float fr );
void bbEntityOrder( Object *o, int n );

//////////////////////////////
// ENTITY PROPERTY COMMANDS //
//////////////////////////////
float bbEntityX( Entity *e, int global );
float bbEntityY( Entity *e, int global );
float bbEntityZ( Entity *e, int global );
float bbEntityPitch( Entity *e, int global );
float bbEntityYaw( Entity *e, int global );
float bbEntityRoll( Entity *e, int global );
float bbGetMatElement( Entity *e, int row, int col );
void bbTFormPoint( float x, float y, float z, Entity *src, Entity *dest );
void bbTFormVector( float x, float y, float z, Entity *src, Entity *dest );
void bbTFormNormal( float x, float y, float z, Entity *src, Entity *dest );
float bbTFormedX();
float bbTFormedY();
float bbTFormedZ();
float bbVectorYaw( float x, float y, float z );
float bbVectorPitch( float x, float y, float z );
float bbDeltaYaw( Entity *src, Entity *dest );
float bbDeltaPitch( Entity *src, Entity *dest );

///////////////////////////////
// ENTITY COLLISION COMMANDS //
///////////////////////////////
void bbResetEntity( Object *o );
static void entityType( Entity *e, int type );
void bbEntityType( Object *o, int type, int recurs );
void bbEntityPickMode( Object *o, int mode, int obs );
Entity * bbGetParent( Entity *e );
int bbGetEntityType( Object *o );
void bbEntityRadius( Object *o, float x_radius, float y_radius );
void bbEntityBox( Object *o, float x, float y, float z, float w, float h, float d );
Object * bbEntityCollided( Object *o, int type );
int bbCountCollisions( Object *o );
float bbCollisionX( Object *o, int index );
float bbCollisionY( Object *o, int index );
float bbCollisionZ( Object *o, int index );
float bbCollisionNX( Object *o, int index );
float bbCollisionNY( Object *o, int index );
float bbCollisionNZ( Object *o, int index );
float bbCollisionTime( Object *o, int index );
Object* bbCollisionEntity( Object *o, int index );
void* bbCollisionSurface( Object *o, int index );
int bbCollisionTriangle( Object *o, int index );
float bbEntityDistance( Entity *src, Entity *dest );

////////////////////////////////////
// ENTITY TRANSFORMATION COMMANDS //
////////////////////////////////////
void bbMoveEntity( Entity *e, float x, float y, float z );
void bbTurnEntity( Entity *e, float p, float y, float r, int global );
void bbTranslateEntity( Entity *e, float x, float y, float z, int global );
void bbPositionEntity( Entity *e, float x, float y, float z, int global );
void bbScaleEntity( Entity *e, float x, float y, float z, int global );
void bbRotateEntity( Entity *e, float p, float y, float r, int global );
void bbPointEntity( Entity *e, Entity *t, float roll );
void bbAlignToVector( Entity *e, float nx, float ny, float nz, int axis, float rate );

//////////////////////////
// ENTITY MISC COMMANDS //
//////////////////////////
void bbNameEntity( Entity *e, BBStr *t );
BBStr* bbEntityName( Entity *e );
BBStr* bbEntityClass( Entity *e );
void bbClearWorld( int e, int b, int t );
extern int active_texs;
int bbActiveTextures();
void blitz3d_open();
void blitz3d_close();
bool blitz3d_create();
bool blitz3d_destroy();

#endif
