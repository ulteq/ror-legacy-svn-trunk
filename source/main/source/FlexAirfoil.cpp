/*
This source file is part of Rigs of Rods
Copyright 2005,2006,2007,2008,2009 Pierre-Michel Ricordel
Copyright 2007,2008,2009 Thomas Fischer

For more information, see http://www.rigsofrods.com/

Rigs of Rods is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3, as 
published by the Free Software Foundation.

Rigs of Rods is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Rigs of Rods.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "FlexAirfoil.h"
#include "ResourceBuffer.h"
#include "approxmath.h"
#include "rormemory.h"

float refairfoilpos[90]={
		0.00, 0.50, 0.00,
		1.00, 0.50, 0.00, 

		0.00, 0.70, 0.03,
		1.00, 0.70, 0.03,
		0.00, 0.30, 0.03,
		1.00, 0.30, 0.03,

		0.00, 0.90, 0.10,
		1.00, 0.90, 0.10,
		0.00, 0.10, 0.10,
		1.00, 0.10, 0.10,

		0.00, 1.00, 0.25,
		1.00, 1.00, 0.25,
		0.00, 0.00, 0.25,
		1.00, 0.00, 0.25,

		0.00, 1.00, 0.50,
		1.00, 1.00, 0.50,
		0.00, 0.00, 0.50,
		1.00, 0.00, 0.50,

		//updated with control surface chord ratio
		0.00, 0.75, 0.75,
		1.00, 0.75, 0.75,
		0.00, 0.25, 0.75,
		1.00, 0.25, 0.75,

		0.00, 0.75, 0.75,
		1.00, 0.75, 0.75,
		0.00, 0.25, 0.75,
		1.00, 0.25, 0.75,

		//moving with control surface
		0.00, 0.50, 1.00,
		1.00, 0.50, 1.00,

		0.00, 0.50, 1.00,
		1.00, 0.50, 1.00
	};

FlexAirfoil::FlexAirfoil(SceneManager *manager, char* name, node_t *nds, int pnfld, int pnfrd, int pnflu, int pnfru, int pnbld, int pnbrd, int pnblu, int pnbru, char* texband, Vector2 texlf, Vector2 texrf, Vector2 texlb, Vector2 texrb, char mtype, float controlratio, float mind, float maxd, char* afname, float lift_coef, AeroEngine** tps, bool break_able)
{
//		innan=0;
	liftcoef=lift_coef;
	breakable=break_able;
	broken=false;
	debug[0]=0;
	free_wash=0;
    smanager=manager;
	aeroengines=tps;
	nodes=nds;
	useInducedDrag=false;
	nfld=pnfld; nodes[nfld].iIsSkin=true;
	nfrd=pnfrd; nodes[nfrd].iIsSkin=true;
	nflu=pnflu; nodes[nflu].iIsSkin=true;
	nfru=pnfru; nodes[nfru].iIsSkin=true;
	nbld=pnbld; nodes[nbld].iIsSkin=true;
	nbrd=pnbrd; nodes[nbrd].iIsSkin=true;
	nblu=pnblu; nodes[nblu].iIsSkin=true;
	nbru=pnbru; nodes[nbru].iIsSkin=true;
	mindef=mind;
	maxdef=maxd;
	airfoil=new Airfoil(afname);
	//airfoil->getcl(-180.0, 0, 0);
	//airfoil->dumpcl();
	int i;
	for (i=0; i<90; i++) airfoilpos[i]=refairfoilpos[i];
	type=mtype;
	hascontrol=(mtype!='n' && mtype!='S'&& mtype!='T' && mtype!='U'&& mtype!='V');
	isstabilator=(mtype=='S' || mtype=='T' || mtype=='U' || mtype=='V');
	stabilleft=(mtype=='T' || mtype=='V');
	deflection=0.0;
	chordratio=controlratio;

	if (hascontrol)
	{
		//setup control surface
		airfoilpos[56]=controlratio;
		airfoilpos[56+3]=controlratio;
		airfoilpos[56+6]=controlratio;
		airfoilpos[56+9]=controlratio;

		airfoilpos[55]=-controlratio+1.5;
		airfoilpos[55+3]=-controlratio+1.5;
		airfoilpos[55+6]=controlratio-0.5;
		airfoilpos[55+9]=controlratio-0.5;
		for (i=0; i<12; i++) airfoilpos[54+12+i]=airfoilpos[54+i];
	}
	/// Create the mesh via the MeshManager
    msh = MeshManager::getSingleton().createManual(name, ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, new ResourceBuffer());

    /// Create submeshes
    subface = msh->createSubMesh();
    subband = msh->createSubMesh();
	subcup=msh->createSubMesh();
	subcdn=msh->createSubMesh();

	//materials
	subface->setMaterialName(texband);
	subband->setMaterialName(texband);
	subcup->setMaterialName(texband);
	subcdn->setMaterialName(texband);

    /// Define the vertices
    nVertices = 24*2+4+2;
    vbufCount = (2*3+2)*nVertices;
	vertices=(float*)ror_malloc(vbufCount*sizeof(float));
	//shadow
	shadownorvertices=(float*)ror_malloc(nVertices*(3+2)*sizeof(float));
	shadowposvertices=(float*)ror_malloc(nVertices*3*2*sizeof(float));

	//textures coordinates
	covertices[0].texcoord=texlf;
	covertices[1].texcoord=texrf;

	covertices[2].texcoord=texlf+(texlb-texlf)*0.03;
	covertices[3].texcoord=texrf+(texrb-texrf)*0.03;
	covertices[4].texcoord=texlf+(texlb-texlf)*0.03;
	covertices[5].texcoord=texrf+(texrb-texrf)*0.03;

	covertices[6].texcoord=texlf+(texlb-texlf)*0.10;
	covertices[7].texcoord=texrf+(texrb-texrf)*0.10;
	covertices[8].texcoord=texlf+(texlb-texlf)*0.10;
	covertices[9].texcoord=texrf+(texrb-texrf)*0.10;

	covertices[10].texcoord=texlf+(texlb-texlf)*0.25;
	covertices[11].texcoord=texrf+(texrb-texrf)*0.25;
	covertices[12].texcoord=texlf+(texlb-texlf)*0.25;
	covertices[13].texcoord=texrf+(texrb-texrf)*0.25;

	covertices[14].texcoord=texlf+(texlb-texlf)*0.45;
	covertices[15].texcoord=texrf+(texrb-texrf)*0.45;
	covertices[16].texcoord=texlf+(texlb-texlf)*0.45;
	covertices[17].texcoord=texrf+(texrb-texrf)*0.45;

	covertices[18].texcoord=texlf+(texlb-texlf)*airfoilpos[56];
	covertices[19].texcoord=texrf+(texrb-texrf)*airfoilpos[56];
	covertices[20].texcoord=texlf+(texlb-texlf)*airfoilpos[56];
	covertices[21].texcoord=texrf+(texrb-texrf)*airfoilpos[56];

	covertices[22].texcoord=covertices[18].texcoord;
	covertices[23].texcoord=covertices[19].texcoord;
	covertices[24].texcoord=covertices[20].texcoord;
	covertices[25].texcoord=covertices[21].texcoord;

	covertices[26].texcoord=texlb;
	covertices[27].texcoord=texrb;
	covertices[28].texcoord=texlb;
	covertices[29].texcoord=texrb;

	for (i=0; i<24; i++) covertices[i+30].texcoord=covertices[i].texcoord;

	/// Define triangles
    /// The values in this table refer to vertices in the above table
    bandibufCount = 3*20;
    faceibufCount = 3*20;
	cupibufCount=3*2;
	cdnibufCount=3*2;
    facefaces=(unsigned short*)ror_malloc(faceibufCount*sizeof(unsigned short));
    bandfaces=(unsigned short*)ror_malloc(bandibufCount*sizeof(unsigned short));
    cupfaces=(unsigned short*)ror_malloc(cupibufCount*sizeof(unsigned short));
    cdnfaces=(unsigned short*)ror_malloc(cdnibufCount*sizeof(unsigned short));
	
	//attack
	bandfaces[0]=0;
	bandfaces[1]=2;
	bandfaces[2]=1;

	bandfaces[3]=2;
	bandfaces[4]=3;
	bandfaces[5]=1;

	bandfaces[6]=0;
	bandfaces[7]=1;
	bandfaces[8]=4;

	bandfaces[9]=4;
	bandfaces[10]=1;
	bandfaces[11]=5;
	for (i=0; i<5; i++)
	{
		//band
		int v=i*4+2;
		if (i!=4)
		{
			bandfaces[i*12+12]=v;
			bandfaces[i*12+13]=v+4;
			bandfaces[i*12+14]=v+1;

			bandfaces[i*12+15]=v+4;
			bandfaces[i*12+16]=v+5;
			bandfaces[i*12+17]=v+1;

			bandfaces[i*12+18]=v+2;
			bandfaces[i*12+19]=v+3;
			bandfaces[i*12+20]=v+6;

			bandfaces[i*12+21]=v+6;
			bandfaces[i*12+22]=v+3;
			bandfaces[i*12+23]=v+7;
		}
/*			if (i==4)
		{
			bandfaces[i*12+20]=v+4;
			bandfaces[i*12+21]=v+4;
			bandfaces[i*12+23]=v+5;
		}
*/

		//sides
		facefaces[i*12]=30+0;
		facefaces[i*12+1]=30+v+4;
		facefaces[i*12+2]=30+v;

		facefaces[i*12+3]=30+0;
		facefaces[i*12+4]=30+v+2;
		facefaces[i*12+5]=30+v+6;

		facefaces[i*12+6]=30+1;
		facefaces[i*12+7]=30+v+1;
		facefaces[i*12+8]=30+v+5;

		facefaces[i*12+9]=30+1;
		facefaces[i*12+10]=30+v+7;
		facefaces[i*12+11]=30+v+3;
		if (i==4)
		{
//				facefaces[i*12+5]=20+v+4;
//				facefaces[i*12+10]=20+v+5;
			facefaces[i*12]=30+0;
			facefaces[i*12+1]=30+v+2;
			facefaces[i*12+2]=30+v;

			facefaces[i*12+3]=30+v+4;
			facefaces[i*12+4]=30+v;
			facefaces[i*12+5]=30+v+2;

			facefaces[i*12+6]=30+1;
			facefaces[i*12+7]=30+v+1;
			facefaces[i*12+8]=30+v+3;

			facefaces[i*12+9]=30+v+5;
			facefaces[i*12+10]=30+v+3;
			facefaces[i*12+11]=30+v+1;
		}

	}
	cupfaces[0]=22;
	cupfaces[1]=26;
	cupfaces[2]=23;
	cupfaces[3]=26;
	cupfaces[4]=27;
	cupfaces[5]=23;

	cdnfaces[0]=24;
	cdnfaces[1]=25;
	cdnfaces[2]=29;
	cdnfaces[3]=24;
	cdnfaces[4]=29;
	cdnfaces[5]=28;

	float tsref=2.0*(nodes[nfrd].RelPosition-nodes[nfld].RelPosition).crossProduct(nodes[nbld].RelPosition-nodes[nfld].RelPosition).length();
	sref=2.0*(nodes[nfrd].RelPosition-nodes[nfld].RelPosition).crossProduct(nodes[nbrd].RelPosition-nodes[nfrd].RelPosition).length();
	if (tsref>sref) sref=tsref;
	sref=sref*sref;

	lratio=(nodes[nfld].RelPosition-nodes[nflu].RelPosition).length()/(nodes[nfld].RelPosition-nodes[nbld].RelPosition).length();
	rratio=(nodes[nfrd].RelPosition-nodes[nfru].RelPosition).length()/(nodes[nfrd].RelPosition-nodes[nbrd].RelPosition).length();

	thickness=(nodes[nfld].RelPosition-nodes[nflu].RelPosition).length();

	//update coords
	updateVertices();

	/// Create vertex data structure for 8 vertices shared between submeshes
    msh->sharedVertexData = new VertexData();
    msh->sharedVertexData->vertexCount = nVertices;

    /// Create declaration (memory format) of vertex data
    decl = msh->sharedVertexData->vertexDeclaration;
    size_t offset = 0;
    decl->addElement(0, offset, VET_FLOAT3, VES_POSITION);
    offset += VertexElement::getTypeSize(VET_FLOAT3);
    decl->addElement(0, offset, VET_FLOAT3, VES_NORMAL);
    offset += VertexElement::getTypeSize(VET_FLOAT3);
//        decl->addElement(0, offset, VET_FLOAT3, VES_DIFFUSE);
//        offset += VertexElement::getTypeSize(VET_FLOAT3);
    decl->addElement(0, offset, VET_FLOAT2, VES_TEXTURE_COORDINATES, 0);
    offset += VertexElement::getTypeSize(VET_FLOAT2);

    /// Allocate vertex buffer of the requested number of vertices (vertexCount) 
    /// and bytes per vertex (offset)
    vbuf = 
      HardwareBufferManager::getSingleton().createVertexBuffer(
          offset, msh->sharedVertexData->vertexCount, HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE);

    /// Upload the vertex data to the card
    vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);

    /// Set vertex buffer binding so buffer 0 is bound to our vertex buffer
    VertexBufferBinding* bind = msh->sharedVertexData->vertexBufferBinding; 
    bind->setBinding(0, vbuf);

    //for the face
	/// Allocate index buffer of the requested number of vertices (ibufCount) 
    HardwareIndexBufferSharedPtr faceibuf = HardwareBufferManager::getSingleton().
     createIndexBuffer(
         HardwareIndexBuffer::IT_16BIT, 
            faceibufCount, 
            HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    /// Upload the index data to the card
    faceibuf->writeData(0, faceibuf->getSizeInBytes(), facefaces, true);

    /// Set parameters of the submesh
    subface->useSharedVertices = true;
    subface->indexData->indexBuffer = faceibuf;
    subface->indexData->indexCount = faceibufCount;
    subface->indexData->indexStart = 0;

    //for the band
	/// Allocate index buffer of the requested number of vertices (ibufCount) 
    HardwareIndexBufferSharedPtr bandibuf = HardwareBufferManager::getSingleton().
     createIndexBuffer(
         HardwareIndexBuffer::IT_16BIT, 
            bandibufCount, 
            HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    /// Upload the index data to the card
    bandibuf->writeData(0, bandibuf->getSizeInBytes(), bandfaces, true);

    /// Set parameters of the submesh
    subband->useSharedVertices = true;
    subband->indexData->indexBuffer = bandibuf;
    subband->indexData->indexCount = bandibufCount;
    subband->indexData->indexStart = 0;

    //for the aileron up
	/// Allocate index buffer of the requested number of vertices (ibufCount) 
    HardwareIndexBufferSharedPtr cupibuf = HardwareBufferManager::getSingleton().
     createIndexBuffer(
         HardwareIndexBuffer::IT_16BIT, 
            cupibufCount, 
            HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    /// Upload the index data to the card
    cupibuf->writeData(0, cupibuf->getSizeInBytes(), cupfaces, true);

    /// Set parameters of the submesh
    subcup->useSharedVertices = true;
    subcup->indexData->indexBuffer = cupibuf;
    subcup->indexData->indexCount = cupibufCount;
    subcup->indexData->indexStart = 0;

    //for the aileron down
	/// Allocate index buffer of the requested number of vertices (ibufCount) 
    HardwareIndexBufferSharedPtr cdnibuf = HardwareBufferManager::getSingleton().
     createIndexBuffer(
         HardwareIndexBuffer::IT_16BIT, 
            cdnibufCount, 
            HardwareBuffer::HBU_STATIC_WRITE_ONLY);

    /// Upload the index data to the card
    cdnibuf->writeData(0, cdnibuf->getSizeInBytes(), cdnfaces, true);

    /// Set parameters of the submesh
    subcdn->useSharedVertices = true;
    subcdn->indexData->indexBuffer = cdnibuf;
    subcdn->indexData->indexCount = cdnibufCount;
    subcdn->indexData->indexStart = 0;
    
    /// Set bounding information (for culling)
    msh->_setBounds(AxisAlignedBox(-20,-20,-20,20,20,20), true);
    //msh->_setBoundingSphereRadius(20.0);

    /// Notify Mesh object that it has been loaded
	//MeshManager::getSingleton().setPrepareAllMeshesForShadowVolumes(false);
	msh->buildEdgeList();
	//msh->prepareForShadowVolume();
	msh->load();
	//MeshManager::getSingleton().setPrepareAllMeshesForShadowVolumes()
}


Vector3 FlexAirfoil::updateVertices()
{
	int i;
	Vector3 center;
	center=nodes[nfld].smoothpos;

	Vector3 vx=nodes[nfrd].smoothpos-nodes[nfld].smoothpos;
	Vector3 vyl=nodes[nflu].smoothpos-nodes[nfld].smoothpos;
	Vector3 vzl=nodes[nbld].smoothpos-nodes[nfld].smoothpos;
	Vector3 vyr=nodes[nfru].smoothpos-nodes[nfrd].smoothpos;
	Vector3 vzr=nodes[nbrd].smoothpos-nodes[nfrd].smoothpos;

	if (breakable) {broken=broken || (vx.crossProduct(vzl).squaredLength()>sref)||(vx.crossProduct(vzr).squaredLength()>sref);}
	else {broken=(vx.crossProduct(vzl).squaredLength()>sref)||(vx.crossProduct(vzr).squaredLength()>sref);}

	Vector3 facenormal=vx;
	facenormal.normalise();

	//control surface
	if (hascontrol)
	{
		float radius=1.0-chordratio;
		airfoilpos[82]=0.5+radius*sin(deflection/57.0)/rratio;
		airfoilpos[79]=0.5+radius*sin(deflection/57.0)/lratio;
		airfoilpos[83]=chordratio+radius*cos(deflection/57.0);
		airfoilpos[80]=airfoilpos[83];
		airfoilpos[89]=airfoilpos[83];
		airfoilpos[88]=airfoilpos[82];
		airfoilpos[86]=airfoilpos[80];
		airfoilpos[85]=airfoilpos[79];
	}

	if (!broken)
	{
		for (i=0; i<30; i++)
		{
			if (i%2)
				covertices[i].vertex=airfoilpos[i*3]*vx+airfoilpos[i*3+1]*vyr+airfoilpos[i*3+2]*vzr;
			else
				covertices[i].vertex=airfoilpos[i*3]*vx+airfoilpos[i*3+1]*vyl+airfoilpos[i*3+2]*vzl;
			if (i<22) covertices[i+30].vertex=covertices[i].vertex;
		}
		covertices[30+22].vertex=covertices[28].vertex;
		covertices[30+23].vertex=covertices[29].vertex;
	}
	else
	{
		for (i=0; i<30; i++)
		{
			if (i%2)
				covertices[i].vertex=airfoilpos[i*3]*Vector3(0.01,0,0)+airfoilpos[i*3+1]*Vector3(0,0.01,0)+airfoilpos[i*3+2]*Vector3(0,0,0.01);
			else
				covertices[i].vertex=airfoilpos[i*3]*Vector3(0.01,0,0)+airfoilpos[i*3+1]*Vector3(0,0.01,0)+airfoilpos[i*3+2]*Vector3(0,0,0.01);
			if (i<22) covertices[i+30].vertex=covertices[i].vertex;
		}
		covertices[30+22].vertex=covertices[28].vertex;
		covertices[30+23].vertex=covertices[29].vertex;
	}

	if (isstabilator)
	{
		//rotate stabilator
		Vector3 rcent, raxis;
		if (!stabilleft)
		{
			rcent=((nodes[nflu].smoothpos+nodes[nbld].smoothpos)/2.0+(nodes[nflu].smoothpos-nodes[nblu].smoothpos)/4.0)-center;
			raxis=(nodes[nflu].smoothpos-nodes[nfld].smoothpos).crossProduct(nodes[nflu].smoothpos-nodes[nblu].smoothpos);
		}
		else
		{
			rcent=((nodes[nfru].smoothpos+nodes[nbrd].smoothpos)/2.0+(nodes[nfru].smoothpos-nodes[nbru].smoothpos)/4.0)-center;
			raxis=(nodes[nfru].smoothpos-nodes[nfrd].smoothpos).crossProduct(nodes[nfru].smoothpos-nodes[nbru].smoothpos);
		}
		raxis.normalise();
		Quaternion rot=Quaternion(Degree(deflection), raxis);
		for (i=0; i<54; i++)
		{
			covertices[i].vertex=rcent+rot*(covertices[i].vertex-rcent);
		}
	}

	//init normals
	for (i=0; i<(int)nVertices; i++)
	{
		covertices[i].normal=Vector3::ZERO;
	}
	//normals
	//accumulate normals per triangle
	for (i=0; i<(int)bandibufCount/3; i++)
	{
		Vector3 v1, v2;
		v1=covertices[bandfaces[i*3+1]].vertex-covertices[bandfaces[i*3]].vertex;
		v2=covertices[bandfaces[i*3+2]].vertex-covertices[bandfaces[i*3]].vertex;
		v1=v1.crossProduct(v2);
		v1.normalise();
//		v1/=3.0;
		covertices[bandfaces[i*3]].normal+=v1;
		covertices[bandfaces[i*3+1]].normal+=v1;
		covertices[bandfaces[i*3+2]].normal+=v1;
	}
	for (i=0; i<(int)cupibufCount/3; i++)
	{
		Vector3 v1, v2;
		v1=covertices[cupfaces[i*3+1]].vertex-covertices[cupfaces[i*3]].vertex;
		v2=covertices[cupfaces[i*3+2]].vertex-covertices[cupfaces[i*3]].vertex;
		v1=v1.crossProduct(v2);
		v1.normalise();
//		v1/=3.0;
		covertices[cupfaces[i*3]].normal+=v1;
		covertices[cupfaces[i*3+1]].normal+=v1;
		covertices[cupfaces[i*3+2]].normal+=v1;
	}
	for (i=0; i<(int)cdnibufCount/3; i++)
	{
		Vector3 v1, v2;
		v1=covertices[cdnfaces[i*3+1]].vertex-covertices[cdnfaces[i*3]].vertex;
		v2=covertices[cdnfaces[i*3+2]].vertex-covertices[cdnfaces[i*3]].vertex;
		v1=v1.crossProduct(v2);
		v1.normalise();
//		v1/=3.0;
		covertices[cdnfaces[i*3]].normal+=v1;
		covertices[cdnfaces[i*3+1]].normal+=v1;
		covertices[cdnfaces[i*3+2]].normal+=v1;
	}
	//normalize
	for (i=0; i<30; i++)
	{
		covertices[i].normal.normalise();
	}

	//for the faces
	for (i=0; i<24; i++) 
		if (i%2) 
			covertices[i+30].normal=facenormal;
		else
			covertices[i+30].normal=-facenormal;
	return center;
}

Vector3 FlexAirfoil::updateShadowVertices()
{
	 int i;
	Vector3 center;
	center=nodes[nfld].smoothpos;

	Vector3 vx=nodes[nfrd].smoothpos-nodes[nfld].smoothpos;
	Vector3 vyl=nodes[nflu].smoothpos-nodes[nfld].smoothpos;
	Vector3 vzl=nodes[nbld].smoothpos-nodes[nfld].smoothpos;
	Vector3 vyr=nodes[nfru].smoothpos-nodes[nfrd].smoothpos;
	Vector3 vzr=nodes[nbrd].smoothpos-nodes[nfrd].smoothpos;

	if (breakable) {broken=broken || (vx.crossProduct(vzl).squaredLength()>sref)||(vx.crossProduct(vzr).squaredLength()>sref);}
	else {broken=(vx.crossProduct(vzl).squaredLength()>sref)||(vx.crossProduct(vzr).squaredLength()>sref);}

	Vector3 facenormal=vx;
	facenormal.normalise();

	//control surface
	if (hascontrol)
	{
		float radius=1.0-chordratio;
		airfoilpos[82]=0.5+radius*sin(deflection/57.0)/rratio;
		airfoilpos[79]=0.5+radius*sin(deflection/57.0)/lratio;
		airfoilpos[83]=chordratio+radius*cos(deflection/57.0);
		airfoilpos[80]=airfoilpos[83];
		airfoilpos[89]=airfoilpos[83];
		airfoilpos[88]=airfoilpos[82];
		airfoilpos[86]=airfoilpos[80];
		airfoilpos[85]=airfoilpos[79];
	}

	if (!broken)
	{
		for (i=0; i<30; i++)
		{
			if (i%2)
				coshadowposvertices[i].vertex=airfoilpos[i*3]*vx+airfoilpos[i*3+1]*vyr+airfoilpos[i*3+2]*vzr;
			else
				coshadowposvertices[i].vertex=airfoilpos[i*3]*vx+airfoilpos[i*3+1]*vyl+airfoilpos[i*3+2]*vzl;
			if (i<22) coshadowposvertices[i+30].vertex=coshadowposvertices[i].vertex;
		}
		coshadowposvertices[30+22].vertex=coshadowposvertices[28].vertex;
		coshadowposvertices[30+23].vertex=coshadowposvertices[29].vertex;
	}
	else
	{
		for (i=0; i<30; i++)
		{
			if (i%2)
				coshadowposvertices[i].vertex=airfoilpos[i*3]*Vector3(0.01,0,0)+airfoilpos[i*3+1]*Vector3(0,0.01,0)+airfoilpos[i*3+2]*Vector3(0,0,0.01);
			else
				coshadowposvertices[i].vertex=airfoilpos[i*3]*Vector3(0.01,0,0)+airfoilpos[i*3+1]*Vector3(0,0.01,0)+airfoilpos[i*3+2]*Vector3(0,0,0.01);
			if (i<22) coshadowposvertices[i+30].vertex=coshadowposvertices[i].vertex;
		}
		coshadowposvertices[30+22].vertex=coshadowposvertices[28].vertex;
		coshadowposvertices[30+23].vertex=coshadowposvertices[29].vertex;
	}

	if (isstabilator)
	{
		//rotate stabilator
		Vector3 rcent, raxis;
		if (!stabilleft)
		{
			rcent=((nodes[nflu].smoothpos+nodes[nbld].smoothpos)/2.0+(nodes[nflu].smoothpos-nodes[nblu].smoothpos)/4.0)-center;
			raxis=(nodes[nflu].smoothpos-nodes[nfld].smoothpos).crossProduct(nodes[nflu].smoothpos-nodes[nblu].smoothpos);
		}
		else
		{
			rcent=((nodes[nfru].smoothpos+nodes[nbrd].smoothpos)/2.0+(nodes[nfru].smoothpos-nodes[nbru].smoothpos)/4.0)-center;
			raxis=(nodes[nfru].smoothpos-nodes[nfrd].smoothpos).crossProduct(nodes[nfru].smoothpos-nodes[nbru].smoothpos);
		}
		raxis.normalise();
		Quaternion rot=Quaternion(Degree(deflection), raxis);
		for (i=0; i<54; i++)
		{
			covertices[i].vertex=rcent+rot*(covertices[i].vertex-rcent);
		}
	}

	//init normals
	for (i=0; i<(int)nVertices; i++)
	{
		coshadowposvertices[i+nVertices]=coshadowposvertices[i];
		coshadownorvertices[i].normal=Vector3::ZERO;
		coshadownorvertices[i].texcoord=covertices[i].texcoord;
	}
	//normals
	//accumulate normals per triangle
	for (i=0; i<(int)bandibufCount/3; i++)
	{
		Vector3 v1, v2;
		v1=coshadowposvertices[bandfaces[i*3+1]].vertex-coshadowposvertices[bandfaces[i*3]].vertex;
		v2=coshadowposvertices[bandfaces[i*3+2]].vertex-coshadowposvertices[bandfaces[i*3]].vertex;
		v1=v1.crossProduct(v2);
		v1.normalise();
//		v1/=3.0;
		coshadownorvertices[bandfaces[i*3]].normal+=v1;
		coshadownorvertices[bandfaces[i*3+1]].normal+=v1;
		coshadownorvertices[bandfaces[i*3+2]].normal+=v1;
	}
	for (i=0; i<(int)cupibufCount/3; i++)
	{
		Vector3 v1, v2;
		v1=coshadowposvertices[cupfaces[i*3+1]].vertex-coshadowposvertices[cupfaces[i*3]].vertex;
		v2=coshadowposvertices[cupfaces[i*3+2]].vertex-coshadowposvertices[cupfaces[i*3]].vertex;
		v1=v1.crossProduct(v2);
		v1.normalise();
//		v1/=3.0;
		coshadownorvertices[cupfaces[i*3]].normal+=v1;
		coshadownorvertices[cupfaces[i*3+1]].normal+=v1;
		coshadownorvertices[cupfaces[i*3+2]].normal+=v1;
	}
	for (i=0; i<(int)cdnibufCount/3; i++)
	{
		Vector3 v1, v2;
		v1=coshadowposvertices[cdnfaces[i*3+1]].vertex-coshadowposvertices[cdnfaces[i*3]].vertex;
		v2=coshadowposvertices[cdnfaces[i*3+2]].vertex-coshadowposvertices[cdnfaces[i*3]].vertex;
		v1=v1.crossProduct(v2);
		v1.normalise();
//		v1/=3.0;
		coshadownorvertices[cdnfaces[i*3]].normal+=v1;
		coshadownorvertices[cdnfaces[i*3+1]].normal+=v1;
		coshadownorvertices[cdnfaces[i*3+2]].normal+=v1;
	}
	//normalize
	for (i=0; i<30; i++)
	{
		coshadownorvertices[i].normal.normalise();
	}

	//for the faces
	for (i=0; i<24; i++) 
		if (i%2) 
			coshadownorvertices[i+30].normal=facenormal;
		else
			coshadownorvertices[i+30].normal=-facenormal;
	return center;
}


void FlexAirfoil::setControlDeflection(float val)
{
	if (val<0) deflection=-val*mindef;
	else deflection=val*maxdef;
}

Vector3 FlexAirfoil::flexit()
{
	Vector3 center;
	if (smanager->getShadowTechnique()==SHADOWTYPE_STENCIL_MODULATIVE || smanager->getShadowTechnique()==SHADOWTYPE_STENCIL_ADDITIVE)
	{
		center=updateShadowVertices();
		//find the binding
		unsigned posbinding=msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_POSITION)->getSource();
		HardwareVertexBufferSharedPtr pbuf=msh->sharedVertexData->vertexBufferBinding->getBuffer(posbinding);
		//pbuf->lock(HardwareBuffer::HBL_NORMAL);
		pbuf->writeData(0, pbuf->getSizeInBytes(), shadowposvertices, true);
		//pbuf->unlock();
		//find the binding
		unsigned norbinding=msh->sharedVertexData->vertexDeclaration->findElementBySemantic(VES_NORMAL)->getSource();
		HardwareVertexBufferSharedPtr nbuf=msh->sharedVertexData->vertexBufferBinding->getBuffer(norbinding);
		//nbuf->lock(HardwareBuffer::HBL_NORMAL);
		nbuf->writeData(0, nbuf->getSizeInBytes(), shadownorvertices, true);
		//nbuf->unlock();

		EdgeData * 	ed=msh->getEdgeList();
		ed->updateFaceNormals(0, pbuf);
	}
		else
	{
		center=updateVertices();
		//vbuf->lock(HardwareBuffer::HBL_NORMAL);
		vbuf->writeData(0, vbuf->getSizeInBytes(), vertices, true);
		//vbuf->unlock();
		//msh->sharedVertexData->vertexBufferBinding->getBuffer(0)->writeData(0, vbuf->getSizeInBytes(), vertices, true);
	}
	return center;
}

void FlexAirfoil::enableInducedDrag(float span, float area, bool l)
{
	idSpan=span;
	idArea=area;
	useInducedDrag=true;
	idLeft=l;
}

void FlexAirfoil::addwash(int propid, float ratio)
{
	washpropnum[free_wash]=propid;
	washpropratio[free_wash]=ratio;
	free_wash++;
}

void FlexAirfoil::updateForces()
{
	if(!airfoil) return;
	if (broken) return;
//	if (innan) {LogManager::getSingleton().logMessage("STEP "+StringConverter::toString(innan)+" "+StringConverter::toString(nblu));innan++;}
	//evaluate wind direction
	Vector3 wind=-(nodes[nfld].Velocity+nodes[nfrd].Velocity)/2.0;
	//add wash
	int i;
	for (i=0; i<free_wash; i++)
		wind-=(0.5*washpropratio[i]*aeroengines[washpropnum[i]]->getpropwash())*aeroengines[washpropnum[i]]->getAxis();
	float wspeed=wind.length();
	//chord vector, front to back
	Vector3 chordv=((nodes[nbld].RelPosition-nodes[nfld].RelPosition)+(nodes[nbrd].RelPosition-nodes[nfrd].RelPosition))/2.0;
	float chord=chordv.length();
	//span vector, left to right
	Vector3 spanv=((nodes[nfrd].RelPosition-nodes[nfld].RelPosition)+(nodes[nbrd].RelPosition-nodes[nbld].RelPosition))/2.0;
	float span=spanv.length();
	//lift vector
//if (_isnan(spanv.x) || _isnan(spanv.y) || _isnan(spanv.z)) LogManager::getSingleton().logMessage("spanv is NaN "+StringConverter::toString(nblu));
//if (_isnan(wind.x) || _isnan(wind.y) || _isnan(wind.z)) LogManager::getSingleton().logMessage("wind is NaN "+StringConverter::toString(nblu));
	Vector3 liftv=spanv.crossProduct(-wind);
//if (_isnan(liftv.x) || _isnan(liftv.y) || _isnan(liftv.z)) LogManager::getSingleton().logMessage("liftv0 is NaN "+StringConverter::toString(nblu));
//if (_isnan(liftv.x) || _isnan(liftv.y) || _isnan(liftv.z)) LogManager::getSingleton().logMessage("liftv1 is NaN "+StringConverter::toString(nblu));

	//wing normal
	float s=span*chord;
	Vector3 normv=chordv.crossProduct(spanv);
	normv.normalise();
	//calculate angle of attack
	Vector3 pwind;
	pwind=Plane(Vector3::ZERO, normv, chordv).projectVector(-wind);
	Vector3 dumb;
	Degree daoa;
	chordv.getRotationTo(-pwind).ToAngleAxis(daoa, dumb);
	aoa=daoa.valueDegrees();
	float raoa=daoa.valueRadians();
	if (dumb.dotProduct(spanv)>0) {aoa=-aoa; raoa=-raoa;};

//if (_isnan(aoa)) LogManager::getSingleton().logMessage("aoa is NaN "+StringConverter::toString(nblu));
	//get airfoil data
	float cz, cx, cm;
	if (isstabilator)
		airfoil->getparams(aoa-deflection, chordratio, 0, &cz, &cx, &cm);
	else
		airfoil->getparams(aoa, chordratio, deflection, &cz, &cx, &cm);
	//compute surface
//if (_isnan(cz)) LogManager::getSingleton().logMessage("cz is NaN "+StringConverter::toString(nblu));
	//float fs=span*(fabs(thickness*cos(raoa))+fabs(chord*sin(raoa)));
	//float ts=span*(fabs(chord*cos(raoa))+fabs(thickness*sin(raoa)));

	//tropospheric model valid up to 11.000m (33.000ft)
	float altitude=nodes[nfld].AbsPosition.y;
	//float sea_level_temperature=273.15+15.0; //in Kelvin (not used)
	float sea_level_pressure=101325; //in Pa
	//float airtemperature=sea_level_temperature-altitude*0.0065; //in Kelvin (not used)
	float airpressure=sea_level_pressure*approx_pow(1.0-0.0065*altitude/288.15, 5.24947); //in Pa
	float airdensity=airpressure*0.0000120896;//1.225 at sea level

	Vector3 wforce=Vector3::ZERO;
	//drag
	wforce=(cx*0.5*airdensity*wspeed*s)*wind;

//if (_isnan(wforce.x) || _isnan(wforce.y) || _isnan(wforce.z)) LogManager::getSingleton().logMessage("wforce1 is NaN "+StringConverter::toString(nblu));
	//induced drag
	if (useInducedDrag) 
	{
		Vector3 idf=(cx*cx*0.25*airdensity*wspeed*idArea*idArea/(3.14159*idSpan*idSpan))*wind;
//if (_isnan(idf.length())) LogManager::getSingleton().logMessage("idf is NaN "+StringConverter::toString(nblu));

		if (idLeft)
		{
			nodes[nblu].Forces+=idf;
			nodes[nbld].Forces+=idf;
		}
		else
		{
			nodes[nbru].Forces+=idf;
			nodes[nbrd].Forces+=idf;
		}
	}

//if (_isnan(wforce.x) || _isnan(wforce.y) || _isnan(wforce.z)) LogManager::getSingleton().logMessage("wforce1a is NaN "+StringConverter::toString(nblu));
//if (_isnan(cz)) LogManager::getSingleton().logMessage("cz is NaN "+StringConverter::toString(nblu));
//if (_isnan(wspeed)) LogManager::getSingleton().logMessage("wspeed is NaN "+StringConverter::toString(nblu));
//if (_isnan(airdensity)) LogManager::getSingleton().logMessage("airdensity is NaN "+StringConverter::toString(nblu));
//if (_isnan(s)) LogManager::getSingleton().logMessage("s is NaN "+StringConverter::toString(nblu));
//if (_isnan(liftv.x) || _isnan(liftv.y) || _isnan(liftv.z)) LogManager::getSingleton().logMessage("liftv is NaN "+StringConverter::toString(nblu));
	//lift
	wforce+=(cz*0.5*airdensity*wspeed*chord)*liftv;


/*if (_isnan(wforce.x) || _isnan(wforce.y) || _isnan(wforce.z)) 
{
	if (innan==0) innan=1;
	LogManager::getSingleton().logMessage("wforce2 is NaN "+StringConverter::toString(nblu));
}
*/
	//moment
	float moment=-cm*0.5*airdensity*wspeed*wspeed*s;//*chord;
	//apply forces

	Vector3 f1=wforce*(liftcoef * 0.75/4.0f)+normv*(liftcoef *moment/(4.0f*0.25f));
	Vector3 f2=wforce*(liftcoef *0.25/4.0f)-normv*(liftcoef *moment/(4.0f*0.75f));

	//focal at 0.25 chord
	nodes[nfld].Forces+=f1;
	nodes[nflu].Forces+=f1;
	nodes[nfrd].Forces+=f1;
	nodes[nfru].Forces+=f1;
	nodes[nbld].Forces+=f2;
	nodes[nblu].Forces+=f2;
	nodes[nbrd].Forces+=f2;
	nodes[nbru].Forces+=f2;



//	sprintf(debug, "wind %i kts, aoa %i, cz %f, vf %f ", (int)(wspeed*1.9438), (int)aoa, cz, normv.y);

}

FlexAirfoil::~FlexAirfoil()
{
	if(airfoil) delete airfoil; airfoil=0;
	if(!msh.isNull()) msh->unload();
}
