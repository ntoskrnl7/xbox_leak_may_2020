#include <max.h>

#ifdef DESIGN_VER
#include "tif_port.h"
#include "tiffio.h"
#include "gtiffio.h"
//#include "geokeys.h"
//#include "geovalues.h"
#include <assert.h>

bool GeoTIFFExtractHdr(TIFFDirectory * td, GTIFF * gt)
{
	// check to make sure we have a valid model transform
	if (!td->td_matrix && !td->td_modpixelscale &&
		!td->td_modties && !td->td_modtrans)
		return false;

	if ((td->td_modties && ! td->td_modpixelscale) ||
		(td->td_modpixelscale && !td->td_modties))
		return false;

	if ((td->td_matrix || td->td_modtrans) && 
		(td->td_modpixelscale || td->td_modties))
		return false;

	// load the gtiff directory header
	gt->version = td->td_geokeydir[0];
	gt->majorRev = td->td_geokeydir[1];
	gt->minorRev = td->td_geokeydir[2];
	gt->numKeys = td->td_geokeydir[3];

	gt->keys = new GTIFFKEYENTRY[gt->numKeys];

	if (!gt->keys)
		return false;

	// load the model tiepoints
	if (td->td_modties)
	{
		gt->model.numTies = td->td_modtiescount / 6;
		gt->model.tiePoints = new Point3[gt->model.numTies*2];
		if (!gt->model.tiePoints)
			return false;

		for (int i = 0; i < gt->model.numTies; i++)
		{
			gt->model.tiePoints[i*2].x = td->td_modties[i*6];
			gt->model.tiePoints[i*2].y = td->td_modties[i*6+1];
			gt->model.tiePoints[i*2].z = td->td_modties[i*6+2];
#ifdef NDEBUG
			gt->model.tiePoints[i*2+1].x = td->td_modties[i*6+3];
			gt->model.tiePoints[i*2+1].y = td->td_modties[i*6+4];
			gt->model.tiePoints[i*2+1].z = td->td_modties[i*6+5];
#else
			gt->model.tiePoints[i*2+1].x = (td->td_modties[i*6+3] - 319165.0f)/100.0f+1200;
			gt->model.tiePoints[i*2+1].y = (td->td_modties[i*6+4] - 4.30938e6)/100.0f+3312;
			gt->model.tiePoints[i*2+1].z = td->td_modties[i*6+5];
#endif
		}
	}

	// load the pixel scale
	if (td->td_modpixelscale)
	{
		gt->model.scale.x = td->td_modpixelscale[0];
		gt->model.scale.y = td->td_modpixelscale[1];
		gt->model.scale.z = td->td_modpixelscale[2];
	}

	// load the model transform
	if (td->td_modtrans)
	{
		Point3 row;
		row.x = td->td_modtrans[0];
		row.y = td->td_modtrans[4];
		row.z = td->td_modtrans[8];
		gt->model.transform.SetRow(0, row);

		row.x = td->td_modtrans[1];
		row.y = td->td_modtrans[5];
		row.z = td->td_modtrans[9];
		gt->model.transform.SetRow(1, row);

		row.x = td->td_modtrans[2];
		row.y = td->td_modtrans[6];
		row.z = td->td_modtrans[10];
		gt->model.transform.SetRow(2, row);

		row.x = td->td_modtrans[3];
		row.y = td->td_modtrans[7];
		row.z = td->td_modtrans[11];
		gt->model.transform.SetRow(3, row);
		gt->model.transform = Inverse(gt->model.transform);
	}
	else if (td->td_matrix)
	{
		Point3 row;
		row.x = td->td_matrix[0];
		row.y = td->td_matrix[4];
		row.z = td->td_matrix[8];
		gt->model.transform.SetRow(0, row);

		row.x = td->td_matrix[1];
		row.y = td->td_matrix[5];
		row.z = td->td_matrix[9];
		gt->model.transform.SetRow(1, row);

		row.x = td->td_matrix[2];
		row.y = td->td_matrix[6];
		row.z = td->td_matrix[10];
		gt->model.transform.SetRow(2, row);

		row.x = td->td_matrix[3];
		row.y = td->td_matrix[7];
		row.z = td->td_matrix[11];
		gt->model.transform.SetRow(3, row);
		gt->model.transform = Inverse(gt->model.transform);
	}
	else if (gt->model.numTies == 1 && td->td_modpixelscale)
	{
		Point3 row;
		row.x = 1.0f / (gt->model.scale.x*gt->width);
		row.y = 0;
		row.z = 0;
		gt->model.transform.SetRow(0, row);

		row.x = 0;
		row.y = -1.0f / (gt->model.scale.y*gt->height);
		row.z = 0;
		gt->model.transform.SetRow(1, row);

		row.x = 0;
		row.y = 0;
		if (gt->model.scale.z > 0.0f)
			row.z = 1.0f / gt->model.scale.z;
		else
			row.z = 0.0f;
		gt->model.transform.SetRow(2, row);

		row.x = -gt->model.tiePoints[1].x/(gt->model.scale.x*gt->width);
		row.y = gt->model.tiePoints[1].y/(gt->model.scale.y*gt->height);
		row.z = -gt->model.tiePoints[1].z;
		gt->model.transform.SetRow(3, row);
	}
	else
		gt->model.transform = Matrix3(1);

	return true;
}

void GeoTIFFExtents(GeoTableItem * data, GTIFF * gt)
{
	Point3 maxPt = Point3(gt->model.tiePoints[1].x+gt->width*gt->model.scale.x,
							gt->model.tiePoints[1].y-gt->height*gt->model.scale.y,
							0.0f);
	data->m_pts[0] = gt->model.tiePoints[1];
	data->m_pts[1] = Point3(maxPt.x, gt->model.tiePoints[1].y, 0.0f);
	data->m_pts[2] = Point3(gt->model.tiePoints[1].x, maxPt.y, 0.0f);
	data->m_pts[3] = maxPt;

	data->m_scale = gt->model.scale;
}

bool GeoTIFFExtractKey(unsigned short * data, TIFFDirectory * td, GTIFFKEYENTRY * gt)
{
	gt->keyID = data[0];
	gt->keyCount = data[2];
	switch (data[1])
	{
	case TIFFTAG_GEODOUBLE:
		gt->keyType = TIF_DOUBLE;
		gt->keyData = (void *)(td->td_geodouble+data[2]);
		break;

	case TIFFTAG_GEOASCII:
		gt->keyType = TIF_ASCII;
		gt->keyData = (void *)(td->td_geoascii+data[2]);
		break;

	case 0:
		gt->keyType = TIF_SHORT;
		gt->keyData = &(data[3]);
		break;

	default:
		gt->keyType = TIF_SHORT;
		gt->keyData = (void *)(td->td_geokeydir+data[2]);
		break;
	}

	return true;
}

GTIFF * GeoTIFFRead(TIFFDirectory * td)
{
	if (!td->td_geokeydir)
		return NULL;

	GTIFF * gt = new GTIFF;

	gt->width = td->td_imagewidth;
	gt->height = td->td_imagelength;

	if (!GeoTIFFExtractHdr(td, gt))
	{
		delete [] gt->keys;
		delete [] gt->model.tiePoints;
		delete gt;
		return NULL;
	}

	for (int i = 0; i < gt->numKeys; i++)
	{
		if (!GeoTIFFExtractKey(&(td->td_geokeydir[(i+1)*4]), td, &(gt->keys[i])))
		{
			delete [] gt->keys;
			delete [] gt->model.tiePoints;
			delete gt;
			return NULL;
		}
	}

	return gt;
}

bool GeoTIFFCoordSysName(GTIFF * gt, char name[256])
{
	assert(gt);

	return true;
}

Matrix3 GeoTIFFModelTransform(GTIFF * gt)
{
	assert(gt);

	return gt->model.transform;
}

void GeoTIFFClose(GTIFF * gt)
{
	if (!gt)
		return;
	delete [] gt->keys;
	delete [] gt->model.tiePoints;
	delete gt;
}

#endif	// DESIGN_VER