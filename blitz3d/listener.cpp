
#include "std.h"
#include "listener.h"

#if BB_FMOD_ENABLED
extern gxAudio *gx_audio;
#else
void bbSet3dListenerConfig(float roll, float dopp, float dist);
void bbSet3dListener(float x,float y, float z, float kx, float ky, float kz, float jx, float jy, float jz, float vx, float vy, float vz);
#endif

Listener::Listener( float roll,float dopp,float dist ){
#if BB_FMOD_ENABLED
	if( !gx_audio ) return;
	gx_audio->set3dOptions( roll,dopp,dist );
#else
	bbSet3dListenerConfig(roll,dopp,dist);
#endif
	renderListener();
}

Listener::Listener( const Listener &t ):
Object(t){
}

Listener::~Listener(){
#if BB_FMOD_ENABLED
	if( !gx_audio ) return;
	Vector pos,vel,up(0,1,0),forward(0,0,1);
	gx_audio->set3dListener( &pos.x,&vel.x,&forward.x,&up.x );
#else
	bbSet3dListener(0,0,0,0,0,1,0,1,0,0,0,0);
#endif
}

void Listener::renderListener(){

	const Vector &pos=getWorldTform().v;
	const Vector &vel=getVelocity();
	const Vector &forward=getWorldTform().m.k.normalized();
	const Vector &up=getWorldTform().m.j.normalized();

#if BB_FMOD_ENABLED
	if(gx_audio) gx_audio->set3dListener( &pos.x,&vel.x,&forward.x,&up.x );
#else
	bbSet3dListener( pos.x, pos.y, pos.z, forward.x,forward.y,forward.z, up.x,up.y,up.z, vel.x,vel.y,vel.z);
#endif
}
