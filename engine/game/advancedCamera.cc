//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------

#include "platform/platform.h"
#include "dgl/dgl.h"
#include "game/game.h"
#include "math/mMath.h"
#include "console/simBase.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "core/bitStream.h"
#include "core/dnet.h"
#include "game/advancedCamera.h"
#include "game/gameConnection.h"
#include "math/mathIO.h"
#include "editor/editor.h"
#include "math/mathUtils.h"
#include "game/moveManager.h"
#include "game/player.h"

#include <string.h>
#include <stdlib.h>

//----------------------------------------------------------------------------
// Defines
//----------------------------------------------------------------------------
#define DISTANCE_FROM_COLLISION 0.4    // move the camera this many meters towards the player from a nearby wall

//----------------------------------------------------------------------------
// Datablock
//----------------------------------------------------------------------------
IMPLEMENT_CO_DATABLOCK_V1(AdvancedCameraData);

AdvancedCameraData::AdvancedCameraData() {
	lookAtOffset.set(0,0,2);
	thirdPersonOffset.set(0,-2,2);
	godViewOffset.set(0,-10,10);
	maxTerrainDiff = 2;
	orbitOffset.set(0, 0, 5);
	orbitMinMaxZoom.set(1,1000);
	orbitMinMaxDeclination.set(10,100);
	damping = 0.25;
}

void AdvancedCameraData::initPersistFields() {
	Parent::initPersistFields();
	addField("lookAtOffset", TypePoint3F, Offset(lookAtOffset, AdvancedCameraData));
	addField("thirdPersonOffset", TypePoint3F, Offset(thirdPersonOffset, AdvancedCameraData));
	addField("godViewOffset", TypePoint3F, Offset(godViewOffset, AdvancedCameraData));
	addField("maxTerrainDiff", TypeF32, Offset(maxTerrainDiff, AdvancedCameraData));
	addField("orbitOffset", TypePoint3F, Offset(orbitOffset, AdvancedCameraData));
	addField("orbitMinMaxZoom", TypePoint2F, Offset(orbitMinMaxZoom, AdvancedCameraData));
	addField("orbitMinMaxDeclination", TypePoint2F, Offset(orbitMinMaxDeclination, AdvancedCameraData));
	addField("damping", TypeF32, Offset(damping, AdvancedCameraData));
}

void AdvancedCameraData::packData(BitStream* stream) {
	Parent::packData(stream);
	stream->write(lookAtOffset.x);
	stream->write(lookAtOffset.y);
	stream->write(lookAtOffset.z);
	stream->write(thirdPersonOffset.x);
	stream->write(thirdPersonOffset.y);
	stream->write(thirdPersonOffset.z);
	stream->write(godViewOffset.x);
	stream->write(godViewOffset.y);
	stream->write(godViewOffset.z);
	stream->write(maxTerrainDiff);
	stream->write(orbitOffset.x);
	stream->write(orbitOffset.y);
	stream->write(orbitOffset.z);
	stream->write(orbitMinMaxZoom.x);
	stream->write(orbitMinMaxZoom.y);
	stream->write(orbitMinMaxDeclination.x);
	stream->write(orbitMinMaxDeclination.y);
	stream->write(damping);
}

void AdvancedCameraData::unpackData(BitStream* stream) {
	Parent::unpackData(stream);
	stream->read(&lookAtOffset.x);
	stream->read(&lookAtOffset.y);
	stream->read(&lookAtOffset.z);
	stream->read(&thirdPersonOffset.x);
	stream->read(&thirdPersonOffset.y);
	stream->read(&thirdPersonOffset.z);
	stream->read(&godViewOffset.x);
	stream->read(&godViewOffset.y);
	stream->read(&godViewOffset.z);
	stream->read(&maxTerrainDiff);
	stream->read(&orbitOffset.x);
	stream->read(&orbitOffset.y);
	stream->read(&orbitOffset.z);
	stream->read(&orbitMinMaxZoom.x);
	stream->read(&orbitMinMaxZoom.y);
	stream->read(&orbitMinMaxDeclination.x);
	stream->read(&orbitMinMaxDeclination.y);
	stream->read(&damping);
}

//----------------------------------------------------------------------------
// Constructor/destructor
//----------------------------------------------------------------------------
IMPLEMENT_CO_NETOBJECT_V1(AdvancedCamera);
Point3F AdvancedCamera::mCameraPos(0, 0, 0);

AdvancedCamera::AdvancedCamera()
{
	mNetFlags.clear(Ghostable);
	mDataBlock = NULL;
	mTypeMask |= CameraObjectType;
	delta.pos = Point3F(0,0,0);
	delta.posVec = VectorF(0,0,0);
	mObjToWorld.setColumn(3,delta.pos);
	mRot = Point3F(0,0,0);

	mPlayerObject = NULL;
	mTargetObject = NULL;
	mFollowTerrain = false;
	mVerticalFreedom = false;

	mDeclination = 45 * M_PI_F / 180;
	mAzimuth = 90 * M_2PI_F / 360;
	mZoomDistance = 10;

	mDamping = 0.6; //How much % smaller the movement will become per second

	mYaw   = 0;
	mPitch = 0;
	mZoom  = 0;

	delta.mZoomDistance = mZoomDistance;
	delta.mDeclination  = mDeclination;
	delta.mAzimuth      = mAzimuth;

	mCurrentLookAtOffset = Point3F(0,0,0);
	mCurrentThirdPersonOffset = Point3F(0,0,0);
	mCurrentGodViewOffset = Point3F(0,0,0);
	mCurrentOrbitOffset       = Point3F(0,0,0);

	mCurrentOrbitMinMaxZoom = Point2F(0,0);
	mCurrentOrbitMinMaxDeclination = Point2F(0,0);

	mMode = ThirdPersonMode;
}

AdvancedCamera::~AdvancedCamera() {
}


//----------------------------------------------------------------------------
// "Standard" code
//----------------------------------------------------------------------------

bool AdvancedCamera::onAdd() {
	if (!Parent::onAdd() || !mDataBlock)
		return false;

	// Set default offsets
	mCurrentLookAtOffset = mDataBlock->lookAtOffset;
	mCurrentThirdPersonOffset = mDataBlock->thirdPersonOffset;
	mCurrentGodViewOffset = mDataBlock->godViewOffset;
	mCurrentOrbitOffset = mDataBlock->orbitOffset;

	// Orbit camera settings
	mCurrentOrbitMinMaxZoom = mDataBlock->orbitMinMaxZoom;
	mCurrentOrbitMinMaxDeclination = mDataBlock->orbitMinMaxDeclination;
	mDamping = mDataBlock->damping;

	resetWorldBox();

	addToScene();

	if (isServerObject())
		scriptOnAdd();

	return true;
}

void AdvancedCamera::onEditorEnable() {
	mNetFlags.set(Ghostable);
}

void AdvancedCamera::onEditorDisable() {
	mNetFlags.clear(Ghostable);
}

void AdvancedCamera::onRemove() {
	scriptOnRemove();
	removeFromScene();
	Parent::onRemove();
}

bool AdvancedCamera::onNewDataBlock(GameBaseData* dptr) {
	mDataBlock = dynamic_cast<AdvancedCameraData*>(dptr);
	if (!mDataBlock || !Parent::onNewDataBlock(dptr))
		return false;

	scriptOnNewDataBlock();
	return true;
}

void AdvancedCamera::onDeleteNotify(SimObject *obj) {
	Parent::onDeleteNotify(obj);
	if (obj == (SimObject*) mPlayerObject) {
		mPlayerObject = NULL;
	}   
	if (obj == (SimObject*) mTargetObject) {
		mTargetObject = NULL;
	}   
}

void AdvancedCamera::getCameraTransform(F32* pos, MatrixF* mat) {
	getEyeTransform(mat);
}

void AdvancedCamera::processTick(const Move* move) {
	Parent::processTick(move);
}

void AdvancedCamera::interpolateTick(F32 dt) {
	Parent::interpolateTick(dt);
}

// Override to ensure both are kept in scope
void AdvancedCamera::onCameraScopeQuery(NetConnection *cr, CameraScopeQuery * query) {
	if (mPlayerObject)
		cr->objectInScope(mPlayerObject);
	Parent::onCameraScopeQuery(cr, query);
}


void AdvancedCamera::initPersistFields() {
	Parent::initPersistFields();
}

void AdvancedCamera::consoleInit() {
}

//----------------------------------------------------------------------------
// The actual camera "meat"
//----------------------------------------------------------------------------

// Update camera position every time time is advanced
void AdvancedCamera::advanceTime(F32 dt) {

	Parent::advanceTime(dt);
	updateMovementValues(dt);

	// This will hold the new position of the camera
	Point3F	cameraPosWorld = mCameraPos;

	// Grab the player and target object used in most modes
	GameBase *castObj = mPlayerObject;
	ShapeBase* playerObj = dynamic_cast<ShapeBase*>(castObj);
	castObj = mTargetObject;
	ShapeBase* targetObj = dynamic_cast<ShapeBase*>(castObj);

	// Update position based on which mode the camera is in
	switch (mMode)
	{
	case TrackMode :
	case StaticMode :
		{
			// Set the camera to its fixed position
			cameraPosWorld = mCameraPos;
			break;
		}

	case ThirdPersonMode :
		{
			// Move camera to third person offset position			
			if(	playerObj != NULL )	{
				MatrixF	objToWorld;
				// If the vertical freedom mode is on, and the player object is a player
				// then we can adjust the tilt of the camera using the head rotation
				Player*	playerObjAsPlayer =	dynamic_cast<Player*>(playerObj);
				if (mVerticalFreedom &&	playerObjAsPlayer != NULL) {

					Point3F	DistanceBehindHead(0, mCurrentThirdPersonOffset.y, 0);
					Point3F	DistanceAboveGround(0, 0, mCurrentThirdPersonOffset.z);

					objToWorld = playerObjAsPlayer->getRenderTransform();
					Point3F	HeadRotation = playerObjAsPlayer->getHeadRotation();
					F32	HeadPitch =	HeadRotation.x;
					MatrixF	HeadPitchMatrix( EulerF( HeadPitch,	0, 0 ) );
					objToWorld.mul(HeadPitchMatrix);

					objToWorld.mulP(DistanceBehindHead,	&cameraPosWorld);
					cameraPosWorld += DistanceAboveGround;
				}
				else {
					objToWorld = playerObj->getRenderTransform();
					objToWorld.mulP(mCurrentThirdPersonOffset, &cameraPosWorld);
				}
			}
			break;
		}  
	case ThirdPersonTargetMode :
		{
			// Calculate vector from player to target, and move camera to offset
			if( playerObj != NULL && targetObj != NULL) {
				MatrixF objToWorld = playerObj->getRenderTransform();
				VectorF dirVec = targetObj->getPosition() - playerObj->getPosition();
				dirVec.normalize();
				objToWorld.setColumn(1, dirVec);			
				objToWorld.mulP(mCurrentThirdPersonOffset, &cameraPosWorld);
			}
			break;
		}
	case GodViewMode :
		{
			// Move camera to track object position
			// and add the offset
			if( playerObj != NULL ) {
				playerObj->getRenderTransform().getColumn(3, &cameraPosWorld);
				cameraPosWorld += mCurrentGodViewOffset;
			}
			break;
		}
	case OrbitMode :
		{
			if ( playerObj != NULL)
			{
				Point3F mConvertedOrbitOffset;
				F32 mOffsetX;
				F32 mOffsetY;
				F32 mOffsetZ;

				MatrixF objToWorld = playerObj->getRenderTransform();
				objToWorld.mulP(mCurrentOrbitOffset, &cameraPosWorld);

				// use spherical to cartesian coord transforms to calculate offset
				mOffsetX = mZoomDistance * sin(mDeclination) * cos(mAzimuth);
				mOffsetY = mZoomDistance * sin(mDeclination) * sin(mAzimuth);
				mOffsetZ = mZoomDistance * cos(mDeclination);
				mConvertedOrbitOffset.set(mOffsetX, mOffsetY, mOffsetZ);
				cameraPosWorld += mConvertedOrbitOffset;
			}
			break;
		}
	default :
		{
			// Somehow we ended up here. Should not happen ever
			Con::errorf("process tick with no mode");
			break;
		}
	}

	// Attempt to follow the terrain if enabled
	if (mFollowTerrain == true && playerObj != NULL) {
		cameraPosWorld = adjustCameraToTerrain(playerObj->getRenderPosition(), cameraPosWorld);
	}

	// Adjust camera position if it collides with "stuff"
	// for all modes except track mode
	if (mMode && mMode != TrackMode && playerObj != NULL) {
		cameraPosWorld = runCameraCollisionCheck(playerObj->getRenderPosition(), cameraPosWorld);
	}

	// Place camera into its final position	
	setPosition(cameraPosWorld);

	// If on the client, calc delta for backstepping
	if (isClientObject()) {
		delta.pos = getPosition();
		delta.rot = getRotation();
		delta.posVec = delta.posVec - delta.pos;
		delta.rotVec = delta.rotVec - delta.rot;
	}

	if (getControllingClient() && mContainer)
		updateContainer();
}

// Try to adjust the camera to follow the terrain. E.g. look "up" when player is waling up a hill
// TODO - This code doesnt work perfectly and should be reworked some day
Point3F AdvancedCamera::adjustCameraToTerrain(const Point3F& playerPos, const Point3F& cameraPos) {
	// Find the height from camera z to terrain z
	U32 mask = TerrainObjectType | InteriorObjectType;
	RayInfo collisionInfo;
	Point3F terrainPos = cameraPos;
	terrainPos.z = -20000;
	Point3F newPos;

	if (gClientContainer.castRay(cameraPos, terrainPos, mask, &collisionInfo)) {

		// Set camera z to be collision point + thirdPersonOffset.z
		// but only if the change is more than the minimum cap limit - HARDCODED TO 0.2
		// and less than the maximum height cap limit difference
		if (mFabs(mCurrentThirdPersonOffset.z - (cameraPos.z - collisionInfo.point.z)) < 0.2) {
			// Dont adjust the camera, as the difference is too small
			newPos = cameraPos;
		} else if (mCurrentThirdPersonOffset.z - (cameraPos.z - collisionInfo.point.z) < mDataBlock->maxTerrainDiff) {
			// Cap the new height by the maximum allowed
			newPos = Point3F(cameraPos.x, cameraPos.y, cameraPos.z - mDataBlock->maxTerrainDiff);
		} else if (mCurrentThirdPersonOffset.z + (cameraPos.z - collisionInfo.point.z) > mDataBlock->maxTerrainDiff) {
			// Cap the new height by the maximum allowed
			newPos = Point3F(cameraPos.x, cameraPos.y, cameraPos.z + mDataBlock->maxTerrainDiff);
		} else {
			// Set camera height to follow terrain
			newPos = Point3F(cameraPos.x, cameraPos.y, collisionInfo.point.z + mCurrentThirdPersonOffset.z);
		}
		return newPos;
	} else {
		return cameraPos;
	}
}

// See if the camera view is ocluded by interiors or terrain, and move the camera closer to the player in that case
Point3F AdvancedCamera::runCameraCollisionCheck(const Point3F& startpos, const Point3F& endpos) {
	U32 mask = InteriorObjectType | TerrainObjectType;
	RayInfo collisionInfo;

	// Pad the start position with the lookAtOffset, so the camera doesnt move strange when a little bump
	// in the terrain is between camera and the player feet

	// Cast ray and check for collision with terrain and interiors
	if (!gClientContainer.castRay(startpos + mCurrentLookAtOffset, endpos, mask, &collisionInfo)) {
		// No collision, so return endpos
		return endpos;
	} else {
		// We collided, so return the point of collision
		// subtract a slight offset so we don�t show clipping
		Point3F LookDir = startpos + mCurrentLookAtOffset - collisionInfo.point;
		LookDir.normalize();
		Point3F AntiClippingOffset = LookDir * DISTANCE_FROM_COLLISION;
		return collisionInfo.point + AntiClippingOffset;
	}
}

// Sets the position and calculates rotation
void AdvancedCamera::setPosition(const Point3F& pos) {

	//Interpolate the camera position
	//(warning, framerate-dependant, must be improved later)
	Point3F tPos = getPosition();
	if (tPos.isZero()) tPos = pos; // avoid starting jitter
	F32 tInterpolation = 1;//0.666666666;
	tPos = pos*tInterpolation + tPos*(1.0-tInterpolation);

	// Set rotation to point at the tracked object
	MatrixF transMat;
	ShapeBase* obj = dynamic_cast<ShapeBase*>(static_cast<SimObject*>(mPlayerObject));
	if (obj && mMode != StaticMode) {
		Point3F objPos;
		getLookAtPos(&objPos);

		VectorF dirVec = objPos - tPos;
		dirVec.normalize();
		MathUtils::getAnglesFromVector(dirVec, mRot.z, mRot.x);
		mRot.x = 0 - mRot.x;

		transMat = MathUtils::createOrientFromDir(dirVec);

	} else {
		MatrixF xRot, zRot;
		// From Teoh Implementation, dun see any effect
		mRot.x = mDeclination;		
		mRot.z = mAzimuth;

		xRot.set(EulerF(mRot.x, 0, 0));
		zRot.set(EulerF(0, 0, mRot.z));
		transMat.mul(zRot, xRot);
	}

	transMat.setColumn(3, tPos);
	Parent::setTransform(transMat);
}

/// Gets the look-at coordinates modified with the lookAtOffset
bool AdvancedCamera::getLookAtPos(Point3F* lookAtPos) {
	ShapeBase* obj = NULL;
	if (mMode == TrackMode || mMode == ThirdPersonMode || mMode == GodViewMode || mMode == OrbitMode) {
		obj = dynamic_cast<ShapeBase*>(static_cast<SimObject*>(mPlayerObject));
	} else if (mMode == ThirdPersonTargetMode) {
		obj = dynamic_cast<ShapeBase*>(static_cast<SimObject*>(mTargetObject));
	}
	if (obj == NULL) {
		return false;
	}

	MatrixF objToWorld = obj->getRenderTransform();
	objToWorld.mulP(mCurrentLookAtOffset, lookAtPos);

	return true;
}

void AdvancedCamera::updateMovementValues(F32 dt)
{
	    //Set the absolute azimuth, declination and zoom values
   if ( dStrcmp("", Con::getVariable("$advCamera::azimuth")) )
      mAzimuth = (M_PI_F / 180) * Con::getFloatVariable( "$advCamera::azimuth", mAzimuth * 180 / M_PI_F ) ;

   if ( dStrcmp("", Con::getVariable("$advCamera::declination")) )
      mDeclination = (M_PI_F / 180) * Con::getFloatVariable( "$advCamera::declination", mDeclination * 180 / M_PI_F );

   if ( dStrcmp("", Con::getVariable("$advCamera::zoomDistance")) )
      mZoomDistance = Con::getFloatVariable( "$advCamera::zoomDistance", mZoomDistance );

   //Clamp the absolute values
   mDeclination = getMax( mCurrentOrbitMinMaxDeclination.x * M_PI_F/180, getMin( mDeclination, mCurrentOrbitMinMaxDeclination.y * M_PI_F/180 ) );
   
   mZoomDistance = getMax( mCurrentOrbitMinMaxZoom.x, getMin( mZoomDistance, mCurrentOrbitMinMaxZoom.y ) );
   
   
   //Reset the console variables that control the absolute values
   Con::setVariable( "$advCamera::azimuth", "" );
   Con::setVariable( "$advCamera::declination", "" );
   Con::setVariable( "$advCamera::zoomDistance", "" );

   //Apply the yaw, pitch and zoom increments
   mYaw   += Con::getFloatVariable( "$advCamera::Yaw", 0.0 ) * dt; //PRLD_TEST
   mPitch += Con::getFloatVariable( "$advCamera::Pitch", 0.0 ) * dt; //PRLD_TEST
   mZoom  += Con::getFloatVariable( "$advCamera::Zoom", 0.0 ) * dt; //PRLD_TEST

   //Reset the increment console variables
   Con::setFloatVariable( "$advCamera::Yaw", 0.0 );
   Con::setFloatVariable( "$advCamera::Pitch", 0.0 );
   Con::setFloatVariable( "$advCamera::Zoom", 0.0 );

	//Determinate the directions being rotated
	F32 mCamRotateLeft  = getMin(mYaw, 0.0f);
	F32 mCamRotateRight = getMax(mYaw, 0.0f);
	F32 mCamRotateUp    = getMin(mPitch, 0.0f);
	F32 mCamRotateDown  = getMax(mPitch, 0.0f);
	F32 mCamZoomIn      = getMin(mZoom, 0.0f);
	F32 mCamZoomOut     = getMax(mZoom, 0.0f);

	if (mCamRotateUp) // top
	{
		F32 newDeclinationAngle;
		// we want to process in degrees during testing for understandability
		// since we use declination, moving the camera "up" is a decrease in angle
		// convert to degrees for now, then apply Change
		newDeclinationAngle = ( mDeclination * 180 / M_PI_F ) + mPitch;
		if (newDeclinationAngle >= mCurrentOrbitMinMaxDeclination.x )	
		{
			mDeclination = newDeclinationAngle * M_PI_F / 180; // converted to radians
		}
		else
		{
			mDeclination = mCurrentOrbitMinMaxDeclination.x * M_PI_F/180;  //set to our MinDeclination define
		}
	}
	if (mCamRotateDown) // bottom
	{
		F32 newDeclinationAngle;
		// we want to process in degrees during testing for understandability
		// since we use declination, moving the camera "down" is an increase in angle
		// convert to degrees for now, then apply Change
		newDeclinationAngle = ( mDeclination * 180 / M_PI_F ) + mPitch;
		if ( newDeclinationAngle <= mCurrentOrbitMinMaxDeclination.y )	
		{
			mDeclination = newDeclinationAngle * M_PI_F / 180; // converted to radians
		}
		else
		{
			mDeclination = mCurrentOrbitMinMaxDeclination.y * M_PI_F/180;  //set to our MinDeclination define
		}			
	}
	if (mCamRotateRight) // right
	{
		F32 newAzimuthAngle;
		// we want to process in degrees during testing for understandability
		// since we use azimuth, moving the camera "right" is an increase in azimuth
		// convert to degrees for now, then apply change
		newAzimuthAngle = (mAzimuth * 180/ M_PI_F) + mYaw;
		if ( newAzimuthAngle < 360 )	// wrap using 360 clock math if needed
		{
			mAzimuth = newAzimuthAngle * M_2PI_F / 360; // converted to radians
		}
		else  // wrap
		{
			mAzimuth = ( newAzimuthAngle - 360 ) *M_2PI_F/360;
		}
	}
	if (mCamRotateLeft) // left
	{
		F32 newAzimuthAngle;
		// we want to process in degrees during testing for understandability
		// since we use azimuth, moving the camera "left" is a decrease in azimuth
		// convert to degrees for now, then apply change
		newAzimuthAngle = (mAzimuth * 180/ M_PI_F) + mYaw;
		if ( newAzimuthAngle > 0 )	// wrap using 360 clock math if needed
		{
			mAzimuth = newAzimuthAngle * M_2PI_F / 360; // converted to radians
		}
		else  // wrap
		{
			mAzimuth = ( newAzimuthAngle + 360 ) *M_2PI_F/360;
		}
	}
	if (mCamZoomIn) // zoom in
	{
		F32 newZoom = mZoomDistance;
		newZoom += mZoom;
		if ( newZoom <= mCurrentOrbitMinMaxZoom.x )
		{
			mZoomDistance = mCurrentOrbitMinMaxZoom.x;
		}
		else
		{
			mZoomDistance = newZoom;
		}	  	
	}
	if (mCamZoomOut) // zoom out
	{
		F32 newZoom = mZoomDistance;
		newZoom += mZoom;
		if ( newZoom >= mCurrentOrbitMinMaxZoom.y )
		{
			mZoomDistance = mCurrentOrbitMinMaxZoom.y;
		}
		else
		{
			mZoomDistance = newZoom;
		}	  	
	}

	//Calculate the time-based damping
	F32 subDamp;
	subDamp = mPow(mDamping, dt*10.0f);

	//Apply damping
	mYaw   *= subDamp;
	mPitch *= subDamp;
	mZoom  *= subDamp;
}

//----------------------------------------------------------------------------
// Networking code
//----------------------------------------------------------------------------
void AdvancedCamera::writePacketData(GameConnection *connection, BitStream *bstream) {
	// Update client regardless of status flags.
	Parent::writePacketData(connection, bstream);

	Point3F pos;
	mObjToWorld.getColumn(3,&pos);
	bstream->setCompressionPoint(pos);
	mathWrite(*bstream, pos);
	mathWrite(*bstream, mCameraPos);
	mathWrite(*bstream, mCurrentLookAtOffset);
	mathWrite(*bstream, mCurrentThirdPersonOffset);
	mathWrite(*bstream, mCurrentGodViewOffset);
	mathWrite(*bstream, mCurrentOrbitOffset);   
	mathWrite(*bstream, mCurrentOrbitMinMaxZoom);
	mathWrite(*bstream, mCurrentOrbitMinMaxDeclination);

	U32 writeMode = mMode;
	bstream->writeRangedU32(writeMode, CameraFirstMode, CameraLastMode);
	bstream->writeFlag(mFollowTerrain);
	bstream->writeFlag(mVerticalFreedom);

	S32 gIndex = bool(mPlayerObject) ? connection->getGhostIndex(mPlayerObject) : -1;
	bstream->writeFlag(gIndex != -1);
	if (gIndex != -1)
		bstream->writeInt(gIndex, NetConnection::GhostIdBitSize);

	gIndex = bool(mTargetObject) ? connection->getGhostIndex(mTargetObject) : -1;
	bstream->writeFlag(gIndex != -1);
	if (gIndex != -1)
		bstream->writeInt(gIndex, NetConnection::GhostIdBitSize);

}

void AdvancedCamera::readPacketData(GameConnection *connection, BitStream *bstream) {
	Parent::readPacketData(connection, bstream);

	Point3F pos;
	mathRead(*bstream, &pos);
	bstream->setCompressionPoint(pos);
	mathRead(*bstream, &mCameraPos);
	mathRead(*bstream, &mCurrentLookAtOffset);
	mathRead(*bstream, &mCurrentThirdPersonOffset);
	mathRead(*bstream, &mCurrentGodViewOffset);
	mathRead(*bstream, &mCurrentOrbitOffset);   
	mathRead(*bstream, &mCurrentOrbitMinMaxZoom);
	mathRead(*bstream, &mCurrentOrbitMinMaxDeclination);
	mMode = bstream->readRangedU32(CameraFirstMode, CameraLastMode);
	mFollowTerrain = bstream->readFlag();
	mVerticalFreedom = bstream->readFlag();

	ShapeBase* playerObj;
	if (bstream->readFlag()) {
		S32 gIndex = bstream->readInt(NetConnection::GhostIdBitSize);
		playerObj = static_cast<ShapeBase*>(connection->resolveGhost(gIndex));
	} else 
		playerObj = 0;

	if (playerObj != (GameBase*) mPlayerObject) {
		if (mPlayerObject) {
			clearProcessAfter();
			clearNotify(mPlayerObject);
		}
		mPlayerObject = playerObj;
		if (mPlayerObject) {
			processAfter(mPlayerObject);
			deleteNotify(mPlayerObject);
		}
	}

	ShapeBase* targetObj;
	if (bstream->readFlag()) {
		S32 gIndex = bstream->readInt(NetConnection::GhostIdBitSize);
		targetObj = static_cast<ShapeBase*>(connection->resolveGhost(gIndex));
	} else 
		targetObj = 0;

	if (targetObj != (GameBase*) mTargetObject) {
		if (mTargetObject) {
			clearProcessAfter();
			clearNotify(mTargetObject);
		}
		mTargetObject = targetObj;
		if (mTargetObject) {
			processAfter(mTargetObject);
			deleteNotify(mTargetObject);
		}
	}
}

U32 AdvancedCamera::packUpdate(NetConnection *con, U32 mask, BitStream *bstream) {
	U32 retMask = Parent::packUpdate(con,mask,bstream);

	// The rest of the data is part of the control object packet update.
	// If we're controlled by this client, we don't need to send it.
	// we only need to send it if this is the initial update - in that case,
	// the client won't know this is the control object yet.
	if(bstream->writeFlag(getControllingClient() == con && !(mask & InitialUpdateMask)))
		return retMask;

	if (bstream->writeFlag(mask & MoveMask)) {
		Point3F pos;
		mObjToWorld.getColumn(3,&pos);
		bstream->write(pos.x);
		bstream->write(pos.y);
		bstream->write(pos.z);
	}

	return retMask;
}

void AdvancedCamera::unpackUpdate(NetConnection *con, BitStream *bstream) {
	Parent::unpackUpdate(con,bstream);

	// controlled by the client?
	if(bstream->readFlag())
		return;

	if (bstream->readFlag()) {
		Point3F pos;
		bstream->read(&pos.x);
		bstream->read(&pos.y);
		bstream->read(&pos.z);
	}
}

//----------------------------------------------------------------------------
// Camera effects
// TODO - do these even work?
//----------------------------------------------------------------------------

F32 AdvancedCamera::getDamageFlash() const {
	if (isServerObject() && bool(mPlayerObject)) {
		const GameBase *castObj = mPlayerObject;
		const ShapeBase* psb = dynamic_cast<const ShapeBase*>(castObj);
		if (psb)
			return psb->getDamageFlash();
	}

	return mDamageFlash;
}

F32 AdvancedCamera::getWhiteOut() const {
	if (isServerObject() && bool(mPlayerObject)) {
		const GameBase *castObj = mPlayerObject;
		const ShapeBase* psb = dynamic_cast<const ShapeBase*>(castObj);
		if (psb)
			return psb->getWhiteOut();
	}

	return mWhiteOut;
}

//----------------------------------------------------------------------------
// Getters/setters
//----------------------------------------------------------------------------

Point3F &AdvancedCamera::getPosition() {
	static Point3F position;
	mObjToWorld.getColumn(3, &position);
	return position;
}

Point3F &AdvancedCamera::getRotation() {
	static Point3F rotation;
	rotation.set(mRot);
	return rotation;
}

Point3F &AdvancedCamera::getCameraPosition() {
	return mCameraPos;
}

void AdvancedCamera::setCameraPosition(const Point3F& pos) {
	mCameraPos = pos;
	setPosition(mCameraPos);
}

void AdvancedCamera::setPlayerObject(GameBase *obj) {
	// reset current object if not null
	if(bool(mPlayerObject)) {
		clearProcessAfter();
		clearNotify(mPlayerObject);
	}
	mPlayerObject = obj;
	if(bool(mPlayerObject)) {
		processAfter(mPlayerObject);
		deleteNotify(mPlayerObject);
	}
}

void AdvancedCamera::setTargetObject(GameBase *obj) {
	// reset current object if not null
	if(bool(mTargetObject)) {
		clearProcessAfter();
		clearNotify(mTargetObject);
	}
	mTargetObject = obj;
	if(bool(mTargetObject)) {
		processAfter(mTargetObject);
		deleteNotify(mTargetObject);
	}
}

void AdvancedCamera::setCameraMode(int mode) {
	// check that the relevant objects are set before switching mode
	if (mode == TrackMode || mode == GodViewMode || mode == ThirdPersonMode || mode == OrbitMode) {
		if (mPlayerObject != NULL) {
			mMode = mode;
		} else {
			Con::errorf("AdvancedCamera: missing player object prior to changing mode");
		}
	} else if (mode == StaticMode) {
			mMode = mode;
	} else if (mode == ThirdPersonTargetMode) {
		if (mPlayerObject != NULL && mTargetObject != NULL) {
			mMode = mode;
		} else {
			Con::errorf("AdvancedCamera: missing player and target object prior to changing mode");
		}
	} else {
		Con::errorf("Unknown camera mode called");
	}
}

void AdvancedCamera::setFollowTerrainMode(bool enable) {
	mFollowTerrain = enable;
}

void AdvancedCamera::setVerticalFreedomMode(bool enable) {
	mVerticalFreedom = enable;
}

void AdvancedCamera::setLookAtOffset(Point3F offset) {
	mCurrentLookAtOffset = offset;
}

void AdvancedCamera::setThirdPersonOffset(Point3F offset) {
	mCurrentThirdPersonOffset = offset;
}

void AdvancedCamera::setGodViewOffset(Point3F offset) {
	mCurrentGodViewOffset = offset;
}

void AdvancedCamera::setOrbitOffset(Point3F offset) {
	mCurrentOrbitOffset = offset;
}

void AdvancedCamera::setOrbitMinMaxZoom(Point2F zoom) {
	mCurrentOrbitMinMaxZoom = zoom;
}

void AdvancedCamera::setOrbitMinMaxDeclination(Point2F declination) {
	mCurrentOrbitMinMaxDeclination = declination;
}

Point3F& AdvancedCamera::getLookAtOffset() {
	return mCurrentLookAtOffset;
}

Point3F& AdvancedCamera::getThirdPersonOffset() {
	return mCurrentThirdPersonOffset;
}

Point3F& AdvancedCamera::getGodViewOffset() {
	return mCurrentGodViewOffset;
}

Point3F& AdvancedCamera::getOrbitOffset() {
	return mCurrentOrbitOffset;
}

Point2F& AdvancedCamera::getOrbitMinMaxZoom() {
	return mCurrentOrbitMinMaxZoom;
}

Point2F& AdvancedCamera::getOrbitMinMaxDeclination() {
	return mCurrentOrbitMinMaxDeclination;
}

//----------------------------------------------------------------------------
// Console methods
//----------------------------------------------------------------------------

ConsoleMethod( AdvancedCamera, getCameraPosition, const char *, 2, 2, "()"
              "Get the position of the camera.\n\n"
              "@returns A string of form \"x y z\", \"rot.x 0 rot.z\".") {
   static char buffer[200];
   AdvancedCamera *camObj = (AdvancedCamera *) object;
   
   Point3F& pos = camObj->getPosition();
   Point3F& rot = camObj->getRotation();
   dSprintf(buffer, sizeof(buffer),"%f %f %f, %f 0 %f",pos.x,pos.y,pos.z,rot.x,rot.z);
   return buffer;
}

ConsoleMethod( AdvancedCamera, setCameraPosition, void, 3, 3, "(Point3F pos)"
              "Set the position of the camera for tracking mode.\n\n") {
   AdvancedCamera *camObj = (AdvancedCamera *) object;

   Point3F pos;
   dSscanf( argv[2], "%f %f %f", &pos.x, &pos.y, &pos.z );
   camObj->setCameraPosition(pos);
}

ConsoleMethod( AdvancedCamera, setPlayerObject, bool, 3, 3, "(GameBase object)") {   
   GameBase *playerObj;
   if(!Sim::findObject(argv[2], playerObj))
   {
		Con::errorf("Object not found");
		return false;
   }
   AdvancedCamera *camObj = (AdvancedCamera *) object;
   camObj->setPlayerObject(playerObj);
   return true;
}

ConsoleMethod( AdvancedCamera, setTargetObject, bool, 3, 3, "(GameBase object)") {   
   GameBase *targetObj;
   if(!Sim::findObject(argv[2], targetObj))
   {
		Con::errorf("Object not found");
		return false;
   }
   AdvancedCamera *camObj = (AdvancedCamera *) object;
   camObj->setTargetObject(targetObj);
   return true;
}

ConsoleMethod( AdvancedCamera, clearPlayerObject, bool, 2, 2, "()") {   
   AdvancedCamera *camObj = (AdvancedCamera *) object;
   camObj->setPlayerObject(NULL);
   return true;
}

ConsoleMethod( AdvancedCamera, clearTargetObject, bool, 2, 2, "()") {   
   AdvancedCamera *camObj = (AdvancedCamera *) object;
   camObj->setTargetObject(NULL);
   return true;
}

ConsoleMethod( AdvancedCamera, setTrackMode, bool, 2, 2, "()") {
   AdvancedCamera *camObj = (AdvancedCamera *) object;
   camObj->setCameraMode(AdvancedCamera::TrackMode);
   return true;
}

ConsoleMethod( AdvancedCamera, setStaticMode, bool, 2, 2, "()") {
   AdvancedCamera *camObj = (AdvancedCamera *) object;
   camObj->setCameraMode(AdvancedCamera::StaticMode);
   return true;
}

ConsoleMethod( AdvancedCamera, setThirdPersonMode, bool, 2, 2, "()") {
   AdvancedCamera *camObj = (AdvancedCamera *) object;
   camObj->setCameraMode(AdvancedCamera::ThirdPersonMode);
   return true;
}

ConsoleMethod( AdvancedCamera, setThirdPersonTargetMode, bool, 2, 2, "()") {
   AdvancedCamera *camObj = (AdvancedCamera *) object;
   camObj->setCameraMode(AdvancedCamera::ThirdPersonTargetMode);
   return true;
}

ConsoleMethod( AdvancedCamera, setGodViewMode, bool, 2, 2, "()") {
   AdvancedCamera *camObj = (AdvancedCamera *) object;
   camObj->setCameraMode(AdvancedCamera::GodViewMode);
   return true;
}

ConsoleMethod( AdvancedCamera, setOrbitMode, bool, 2, 2, "()") {
   AdvancedCamera *camObj = (AdvancedCamera *) object;
   camObj->setCameraMode(AdvancedCamera::OrbitMode);
   return true;
}

ConsoleMethod( AdvancedCamera, setFollowTerrainMode, bool, 3, 3, "(bool enable)") {
   AdvancedCamera *camObj = (AdvancedCamera *) object;
   camObj->setFollowTerrainMode(dAtob(argv[2]));
   return true;
}

ConsoleMethod( AdvancedCamera, setVerticalFreedomMode, bool, 3, 3, "(bool enable)") {
    AdvancedCamera *camObj = (AdvancedCamera *) object;
    camObj->setVerticalFreedomMode(dAtob(argv[2]));
    return true;
}

ConsoleMethod( AdvancedCamera, setLookAtOffset, void, 3, 3, "(Point3F offset)"
              "Set the look at offset.\n\n") {
   AdvancedCamera *camObj = (AdvancedCamera *) object;

   Point3F pos;
   dSscanf( argv[2], "%f %f %f", &pos.x, &pos.y, &pos.z );
   camObj->setLookAtOffset(pos);
}

ConsoleMethod( AdvancedCamera, setThirdPersonOffset, void, 3, 3, "(Point3F offset)"
              "Set the third person offset.\n\n") {
   AdvancedCamera *camObj = (AdvancedCamera *) object;

   Point3F pos;
   dSscanf( argv[2], "%f %f %f", &pos.x, &pos.y, &pos.z );
   camObj->setThirdPersonOffset(pos);
}

ConsoleMethod( AdvancedCamera, setGodViewOffset, void, 3, 3, "(Point3F offset)"
              "Set the god view offset.\n\n") {
   AdvancedCamera *camObj = (AdvancedCamera *) object;

   Point3F pos;
   dSscanf( argv[2], "%f %f %f", &pos.x, &pos.y, &pos.z );
   camObj->setGodViewOffset(pos);
}

ConsoleMethod( AdvancedCamera, setOrbitOffset, void, 3, 3, "(Point3F offset)"
              "Set the orbit view offset.\n\n") {
   AdvancedCamera *camObj = (AdvancedCamera *) object;

   Point3F pos;
   dSscanf( argv[2], "%f %f %f", &pos.x, &pos.y, &pos.z );
   camObj->setOrbitOffset(pos);
}

ConsoleMethod( AdvancedCamera, getLookAtOffset, const char *, 2, 2, "()"
              "Get the current look at offset.\n\n"
              "@returns A string of form \"x y z\".") {
   static char buffer[200];
   AdvancedCamera *camObj = (AdvancedCamera *) object;
   
   Point3F& pos = camObj->getLookAtOffset();
   dSprintf(buffer, sizeof(buffer),"%f %f %f",pos.x,pos.y,pos.z);
   return buffer;
}

ConsoleMethod( AdvancedCamera, getThirdPersonOffset, const char *, 2, 2, "()"
              "Get the current third person offset.\n\n"
              "@returns A string of form \"x y z\".") {
   static char buffer[200];
   AdvancedCamera *camObj = (AdvancedCamera *) object;
   
   Point3F& pos = camObj->getThirdPersonOffset();
   dSprintf(buffer, sizeof(buffer),"%f %f %f",pos.x,pos.y,pos.z);
   return buffer;
}

ConsoleMethod( AdvancedCamera, getGodViewOffset, const char *, 2, 2, "()"
              "Get the current god view offset.\n\n"
              "@returns A string of form \"x y z\".") {
   static char buffer[200];
   AdvancedCamera *camObj = (AdvancedCamera *) object;
   
   Point3F& pos = camObj->getGodViewOffset();
   dSprintf(buffer, sizeof(buffer),"%f %f %f",pos.x,pos.y,pos.z);
   return buffer;
}

ConsoleMethod( AdvancedCamera, getOrbitOffset, const char *, 2, 2, "()"
              "Get the current orbit view offset.\n\n"
              "@returns A string of form \"x y z\".") {
   static char buffer[200];
   AdvancedCamera *camObj = (AdvancedCamera *) object;
   
   Point3F& pos = camObj->getOrbitOffset();
   dSprintf(buffer, sizeof(buffer),"%f %f %f",pos.x,pos.y,pos.z);
   return buffer;
}

ConsoleMethod( AdvancedCamera, setOrbitMinMaxZoom, void, 3, 3, "(Point2F zoom)"
              "Set the min and max zoom for orbit mode.\n\n") {
   AdvancedCamera *camObj = (AdvancedCamera *) object;

   Point2F zoom;
   dSscanf( argv[2], "%f %f", &zoom.x, &zoom.y );
   camObj->setOrbitMinMaxZoom(zoom);
}

ConsoleMethod( AdvancedCamera, setOrbitMinMaxDeclination, void, 3, 3, "(Point2F declination)"
              "Set the min and max declination for orbit mode.\n\n") {
   AdvancedCamera *camObj = (AdvancedCamera *) object;

   Point2F dec;
   dSscanf( argv[2], "%f %f", &dec.x, &dec.y );
   camObj->setOrbitMinMaxDeclination(dec);
}
