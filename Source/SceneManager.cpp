///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	// clear the allocated memory
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
	// destroy the created OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

 /***********************************************************
  *  LoadSceneTextures()
  *
  *  This method is used for preparing the 3D scene by loading
  *  the shapes, textures in memory to support the 3D scene
  *  rendering
  ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	bool bReturn = false;

	bReturn = CreateGLTexture(
		"textures/floor.png",
		"floor");

	bReturn = CreateGLTexture(
		"textures/metal.jpg",
		"metal");

	bReturn = CreateGLTexture(
		"textures/wood.jpg",
		"wood");

	bReturn = CreateGLTexture(
		"textures/wall.jpg",
		"wall");

	bReturn = CreateGLTexture(
		"textures/notepad.png",
		"notepad");

	bReturn = CreateGLTexture(
		"textures/cover.jpg",
		"cover");

	bReturn = CreateGLTexture(
		"textures/cover2.jpg",
		"cover2");

	bReturn = CreateGLTexture(
		"textures/notebookspine.png",
		"notebookspine");

	bReturn = CreateGLTexture(
		"textures/pages.png",
		"pages");

	bReturn = CreateGLTexture(
		"textures/plastic.png",
		"plastic");

	bReturn = CreateGLTexture(
		"textures/pencil.png",
		"pencil");

	bReturn = CreateGLTexture(
		"textures/pencil2.png",
		"pencil2");

	bReturn = CreateGLTexture(
		"textures/penciltop.png",
		"penciltop");

	bReturn = CreateGLTexture(
		"textures/penciltop2.png",
		"penciltop2");

	bReturn = CreateGLTexture(
		"textures/clay.jpg",
		"clay");

	bReturn = CreateGLTexture(
		"textures/claytop.png",
		"claytop");


	// after the texture image data is loaded into memory, the
	// loaded textures need to be bound to texture slots - there
	// are a total of 16 available slots for scene textures
	BindGLTextures();
}

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects within the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	OBJECT_MATERIAL goldMaterial;
	goldMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.ambientStrength = 0.3f;
	goldMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	goldMaterial.specularColor = glm::vec3(0.5f, 0.5f, 0.5f);
	goldMaterial.shininess = 35.0;
	goldMaterial.tag = "metal";

	m_objectMaterials.push_back(goldMaterial);

	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.ambientStrength = 0.2f;
	woodMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	woodMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	woodMaterial.shininess = 100.0;
	woodMaterial.tag = "wood";

	m_objectMaterials.push_back(woodMaterial);

	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.4f, 0.4f, 0.4f);
	glassMaterial.ambientStrength = 0.3f;
	glassMaterial.diffuseColor = glm::vec3(0.3f, 0.3f, 0.3f);
	glassMaterial.specularColor = glm::vec3(0.6f, 0.6f, 0.6f);
	glassMaterial.shininess = 85.0;
	glassMaterial.tag = "glass";

	m_objectMaterials.push_back(glassMaterial);

	OBJECT_MATERIAL cheeseMaterial;
	cheeseMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	cheeseMaterial.ambientStrength = 0.2f;
	cheeseMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	cheeseMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	cheeseMaterial.shininess = 0.3;
	cheeseMaterial.tag = "cover";

	m_objectMaterials.push_back(cheeseMaterial);

	OBJECT_MATERIAL breadMaterial;
	breadMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	breadMaterial.ambientStrength = 0.3f;
	breadMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	breadMaterial.specularColor = glm::vec3(0.3f, 0.3f, 0.3f);
	breadMaterial.shininess = 0.5;
	breadMaterial.tag = "bread";

	m_objectMaterials.push_back(breadMaterial);

	OBJECT_MATERIAL darkBreadMaterial;
	darkBreadMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.2f);
	darkBreadMaterial.ambientStrength = 0.2f;
	darkBreadMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	darkBreadMaterial.specularColor = glm::vec3(0.0f, 0.0f, 0.0f);
	darkBreadMaterial.shininess = 0.0;
	darkBreadMaterial.tag = "darkbread";

	m_objectMaterials.push_back(darkBreadMaterial);

	OBJECT_MATERIAL backdropMaterial;
	backdropMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	backdropMaterial.ambientStrength = 0.6f;
	backdropMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	backdropMaterial.specularColor = glm::vec3(0.95f, 0.85f, 0.8f);
	backdropMaterial.shininess = 2.0;
	backdropMaterial.tag = "backdrop";

	m_objectMaterials.push_back(backdropMaterial);

	OBJECT_MATERIAL grapeMaterial;
	grapeMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	grapeMaterial.ambientStrength = 0.1f;
	grapeMaterial.diffuseColor = glm::vec3(0.2f, 0.2f, 0.2f);
	grapeMaterial.specularColor = glm::vec3(0.95f, 0.85f, 0.8f);
	grapeMaterial.shininess = 0.5;
	grapeMaterial.tag = "grape";

	m_objectMaterials.push_back(grapeMaterial);

	OBJECT_MATERIAL lizardMaterial;
	lizardMaterial.ambientColor = glm::vec3(0.1f, 0.1f, 0.1f);
	lizardMaterial.ambientStrength = 0.2f;
	lizardMaterial.diffuseColor = glm::vec3(0.95f, 0.85f, 0.8f);
	lizardMaterial.specularColor = glm::vec3(0.95f, 0.85f, 0.8f);
	lizardMaterial.shininess = 0.3;
	lizardMaterial.tag = "plastic";

	m_objectMaterials.push_back(lizardMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.  There are up to 4 light sources.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Enable custom lighting in shaders
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// First light source (Point light with amber tones)
	m_pShaderManager->setVec3Value("lightSources[0].position", -2.5f, 4.5f, 6.5f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.2f, 0.15f, 0.1f); // Amber ambient
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.7f, 0.5f, 0.3f); // Amber diffuse
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.25f, 0.2f, 0.1f); // Amber specular
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.5f);

	// Second light source (Point light with a brighter tone)
	m_pShaderManager->setVec3Value("lightSources[1].position", -0.21f, 3.29f, 0.6f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.25f, 0.2f, 0.15f); // Warm amber
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.85f, 0.7f, 0.5f); // Bright amber diffuse
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.8f, 0.7f, 0.6f); // Bright specular
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 1.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.8f);


	// Directional light
	m_pShaderManager->setVec3Value("dirLight.direction", 1.0f, -1.0f, 0.0f);  // Direction from top-right
	m_pShaderManager->setVec3Value("dirLight.ambient", 0.2f, 0.2f, 0.2f);    // Soft ambient light
	m_pShaderManager->setVec3Value("dirLight.diffuse", 0.7f, 0.7f, 0.7f);    // Bright diffuse
	m_pShaderManager->setVec3Value("dirLight.specular", 1.0f, 1.0f, 1.0f);   // Strong specular

}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the texture image files for the textures applied
	// to objects in the 3D scene
	LoadSceneTextures();
	// define the materials that will be used for the objects
	// in the 3D scene
	DefineObjectMaterials();
	// add and defile the light sources for the 3D scene
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadCylinderMesh();
	m_basicMeshes->LoadConeMesh();
	m_basicMeshes->LoadPrismMesh();
	m_basicMeshes->LoadPyramid4Mesh();
	m_basicMeshes->LoadSphereMesh();
	m_basicMeshes->LoadTaperedCylinderMesh();
	m_basicMeshes->LoadTorusMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	/******************************************************************/
	/******************************************************************/
	// Draw Box (table top)
	// SetShaderColor(0.5f, 0.25f, 0.1f, 1.0f); // Color

	// Set the scale for the table top
	scaleXYZ = glm::vec3(9.0f, 0.5f, 4.91f); // Scale
	// Set the transformations (position and rotation) for the table top
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(-2.0f, 0.0f, -0.5f)); // position

	// Apply the texture for the table top
	SetShaderTexture("wood");
	SetTextureUVScale(3.0f, 1.5f);
	SetShaderMaterial("wood");

	// Draw the table top
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// Draw Box (table leg 1)
	//SetShaderColor(0.5f, 0.25f, 0.1f, 1.0f); // Brown color for the table legs

	// Set the scale for the first leg
	scaleXYZ = glm::vec3(0.3f, 4.0f, 0.3f); // Larger and taller leg

	// Set the transformations (position) for the first leg
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(-6.3f, -2.0f, -2.8f)); // position
	
	SetShaderColor(0.4f, 0.2f, 0.1f, 1.0f); // Darker brown color for the legs
	SetShaderMaterial("wood");

	// Draw the first leg
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	// Draw Box (table leg 2)
	scaleXYZ = glm::vec3(0.3f, 4.0f, 0.3f); // Same scale as the first leg

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(2.3f, -2.0f, -2.8f)); // position

	SetShaderColor(0.4f, 0.2f, 0.1f, 1.0f); // Darker brown color for the legs
	SetShaderMaterial("wood");
	

	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// Draw Box (table leg 3)
	scaleXYZ = glm::vec3(0.3f, 4.0f, 0.3f); // Same scale as the first leg

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(-6.3f, -2.0f, 1.8f)); // position

	SetShaderColor(0.4f, 0.2f, 0.1f, 1.0f); // Darker brown color for the legs
	SetShaderMaterial("wood");

	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// Draw Box (table leg 4)
	scaleXYZ = glm::vec3(0.3f, 4.0f, 0.3f); // Same scale as the first leg

	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(2.3f, -2.0f, 1.8f)); // position

	SetShaderColor(0.4f, 0.2f, 0.1f, 1.0f); // Darker brown color for the legs
	SetShaderMaterial("wood");

	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	
	/******************************************************************/
	// Draw Plane (wall)
	// Set the shader color for the back wall
	SetShaderColor(0.6f, 0.6f, 0.6f, 1.0f); // Greyish color

	// Set the scale for the back wall mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f); 

	// Set the transformations (position and rotation) for the back wall
	SetTransformations(scaleXYZ, 90.0f, 0.0f, 0.0f, glm::vec3(0.0f, 2.0f, -3.0f));

	// Apply the texture for tiling
	SetShaderTexture("wall"); 	
	SetTextureUVScale(3.0f, 3.0f); // tiling amount
	SetShaderMaterial("wood");


	// Draw the back wall
	m_basicMeshes->DrawPlaneMesh();


	/******************************************************************/
	// Draw floor

	// Set the scale for the back wall mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// Set the transformations (position and rotation) for the back wall
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, -4.0f, 4.0f));

	// Apply the texture for tiling
	SetShaderTexture("floor");
	SetTextureUVScale(3.0f, 3.0f); // tiling amount
	SetShaderMaterial("grape");


	// Draw the floor
	m_basicMeshes->DrawPlaneMesh();

	/******************************************************************/
	// Draw notepad

	// Set the scale for the back wall mesh
	scaleXYZ = glm::vec3(3.0f, 3.5f, 0.05f);

	// Set the transformations (position and rotation) for the back wall
	SetTransformations(scaleXYZ, -7.0f, 180.0f, 0.0f, glm::vec3(-1.9f, 1.9f, -2.75f));

	// Apply the texture for tiling
	SetShaderTexture("notepad");
	SetTextureUVScale(1.0f, 1.0f); // tiling amount
	SetShaderMaterial("backdrop");

	// Draw the notepad
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	// Draw pencil holder
	// Render the top of the pencil case with the hole texture
	SetShaderColor(0.8f, 0.6f, 0.5f, 1.0f); // Same color as the base
	scaleXYZ = glm::vec3(0.4f, 0.05f, 0.3f); // Thin top face for the cylinder
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(1.3f, 1.161f, 1.0f)); // Top position of the cylinder

	// Apply texture and draw the top face
	SetShaderTexture("claytop");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawCylinderMesh();

	// Draw Cylinder (pencil holder) base
	SetShaderColor(0.8f, 0.6f, 0.5f, 1.0f); // Light brown/orange color
	scaleXYZ = glm::vec3(0.41f, 1.21f, 0.31f); // Set the XYZ scale for the mesh
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(1.3f, 0.0f, 1.0f)); // Set the transformations

	// Apply texture and draw the base mesh
	SetShaderTexture("clay");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawCylinderMesh();

	/******************************************************************/
    // Draw Cylinder (lamp base)
	// SetShaderColor(0.95f, 0.85f, 0.8f, 1.0f); // Off white color
	scaleXYZ = glm::vec3(1.0f, 0.4f, 0.7f);// set the XYZ scale for the mesh
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(0.0f, 0.25f, 0.0f)); // set the transformations
	// Texture
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawCylinderMesh();

	/******************************************************************/
	// Draw Cylinder (lamp stem)
	// SetShaderColor(0.4f, 0.4f, 0.4f, 1.0f); // Dark grey color
	scaleXYZ = glm::vec3(0.1f, 4.0f, 0.1f);// set the XYZ scale for the mesh
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(0.6f, 0.35f, 0.0f)); // set the transformations
	// Texture
	SetShaderTexture("metal");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("metal");

	m_basicMeshes->DrawCylinderMesh();

	/******************************************************************/
	// Draw Cone (lamp shade)
	SetShaderColor(0.95f, 0.85f, 0.8f, 1.0f); // Off white color
	scaleXYZ = glm::vec3(0.6f, 1.0f, 0.6f); // Scale for the cone
	SetTransformations(scaleXYZ, -45.0f, 0.0f, -20.0f, glm::vec3(-0.2f, 3.65f, 0.6f)); // set the transformations
	// Texture
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawConeMesh();

	/******************************************************************/
	// Draw Cylinder (lamp top)
	SetShaderColor(0.95f, 0.85f, 0.8f, 1.0f); // Off white color
	scaleXYZ = glm::vec3(0.16f, 0.7f, 0.16f);// set the XYZ scale for the mesh
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 90.0f, glm::vec3(0.9f, 4.25f, 0.0f)); // set the transformations
	// Texture
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");

	m_basicMeshes->DrawCylinderMesh();

	/******************************************************************/
	// Draw Cylinder (lamp top 2)
	SetShaderColor(0.95f, 0.85f, 0.8f, 1.0f); // Off white color
	scaleXYZ = glm::vec3(0.2f, 0.8f, 0.2f);// set the XYZ scale for the mesh
	SetTransformations(scaleXYZ, -45.0f, 0.0f, -20.0f, glm::vec3(0.0f, 3.95f, 0.3f)); // set the transformations
	// Texture
	SetShaderTexture("plastic");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("wood");
	m_basicMeshes->DrawCylinderMesh();


	/******************************************************************/
	// Draw Box (book 1)
	//SetShaderColor(0.6f, 0.2f, 0.2f, 1.0f); // Red color

	// Set the scale for book 1
	scaleXYZ = glm::vec3(2.0f, 0.15f, 1.5f);

	// Set the transformations (position and no rotation) for book 1
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(-4.0f, 0.329f, -1.5f)); // No rotation applied

	// Apply the texture for book 1
	SetShaderTexture("cover2");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("cover");

	// Draw book 1
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	// Draw Box (book 1 pages)
	
	// Set the scale for book 1 pages
	scaleXYZ = glm::vec3(1.98f, 0.12f, 1.501f);

	// Set the transformations (position and no rotation) for book 1 pages
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(-3.985f, 0.329f, -1.5f)); // No rotation applied

	// Apply the texture for book 1 pages
	SetShaderTexture("pages");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");
	 
	// Draw book 1 pages
	m_basicMeshes->DrawBoxMesh();


	/******************************************************************/
	// Draw Box (book 2)
	//SetShaderColor(0.6f, 0.2f, 0.2f, 1.0f); // Red color

	// Set the scale for book 2
	scaleXYZ = glm::vec3(2.0f, 0.25f, 1.5f);

	// Set the transformations (position and no rotation) for book 2
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(-3.99f, 0.53f, -1.5f)); // No rotation applied

	// Apply the texture for book 2
	SetShaderTexture("cover2");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("cover");

	// Draw book 2
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	// Draw Box (book 2 pages)
	//SetShaderColor(0.6f, 0.2f, 0.2f, 1.0f); // Red color

	// Set the scale for book 2 pages
	scaleXYZ = glm::vec3(1.97f, 0.23f, 1.501f);

	// Set the transformations (position and no rotation) for book 2 pages
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(-3.97f, 0.53f, -1.5f)); // No rotation applied

	// Apply the texture for book 2 pages
	SetShaderTexture("pages");
	SetTextureUVScale(1.0f, 0.5f);
	SetShaderMaterial("wood");

	// Draw book 2 pages
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	// Draw Box (book 3)
	SetShaderColor(0.6f, 0.2f, 0.2f, 1.0f); // Red color

	// Set the scale for book 3
	scaleXYZ = glm::vec3(2.0f, 0.45f, 1.5f);

	// Set the transformations (position and no rotation) for book 3
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(-4.01f, 0.88f, -1.5f)); // No rotation applied

	// Apply the texture for book 3
	SetShaderTexture("cover2");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("cover");

	// Draw book 3
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	// Draw Box (book 3 pages)
	//SetShaderColor(0.6f, 0.2f, 0.2f, 1.0f); // Red color

	// Set the scale for book 3 pages
	scaleXYZ = glm::vec3(1.955f, 0.40f, 1.501f);

	// Set the transformations (position and no rotation) for book 3 pages
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(-3.98f, 0.88f, -1.5f)); // No rotation applied

	// Apply the texture for book 3 pages
	SetShaderTexture("pages");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");

	// Draw book 3 pages
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	//  Bowl on top of books
	SetShaderColor(0.8f, 0.6f, 0.5f, 1.0f); // Same color as the base
	scaleXYZ = glm::vec3(0.5f, 0.05f, 0.3f); // Thin top face for the cylinder
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(-3.8f, 1.451f, -1.3f)); // Top position of the cylinder

	// Apply texture and draw the top face
	SetShaderTexture("claytop");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("cover");

	m_basicMeshes->DrawCylinderMesh();

	// Draw Cylinder (bowl) base
	SetShaderColor(0.8f, 0.6f, 0.5f, 1.0f); // Light brown/orange color
	scaleXYZ = glm::vec3(0.51f, 0.6f, 0.31f); // Set the XYZ scale for the mesh
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(-3.8f, 0.9f,-1.3f)); // Set the transformations

	// Apply texture and draw the base mesh
	SetShaderTexture("clay");
	SetTextureUVScale(1.0, 1.0);
	SetShaderMaterial("cover");

	m_basicMeshes->DrawCylinderMesh();

	/******************************************************************/
    // Draw Box (book 4 - Top/Bottom)
    SetShaderColor(0.8f, 0.3f, 0.1f, 1.0f); // Orange color

    // Set the scale for book 4
    scaleXYZ = glm::vec3(1.0f, 0.04f, 1.2f);

    // Set the transformations (position and rotation) for book 4
    SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(-2.1f, 0.2751f, 1.0f));

    // Apply the texture for book 4
	SetShaderTexture("cover");
    SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("backdrop");

    // Draw book 4
    m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	// Draw Box (book 4 - Spine)
	SetShaderColor(0.8f, 0.3f, 0.1f, 1.0f); // Orange color

	// Set the scale for book 4 spine
	scaleXYZ = glm::vec3(1.0f, 0.04f, 1.2f);

	// Set the transformations (position and rotation) for book 4
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(-2.11f, 0.275f, 1.0f));

	// Apply the texture for book 4 spine
	SetShaderTexture("notebookspine");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("backdrop");

	// Draw book 4 spine
	m_basicMeshes->DrawBoxMesh();
	
	/******************************************************************/
	// Draw Box (book 4 - pages)
	SetShaderColor(0.8f, 0.3f, 0.1f, 1.0f); // Orange color

	// Set the scale for book 4 pages
	scaleXYZ = glm::vec3(1.0f, 0.039f, 1.201f);

	// Set the transformations (position and rotation) for book 4
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(-2.099f, 0.2751f, 1.0f));

	// Apply the texture for book 4 pages
	SetShaderTexture("pages");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");

	// Draw book 4 pages
	m_basicMeshes->DrawBoxMesh();

    /******************************************************************/
    // Draw Box (book 5 - top)
    SetShaderColor(0.3f, 0.7f, 0.4f, 1.0f); // Light green color

    // Set the scale for book 5
    scaleXYZ = glm::vec3(1.1f, 0.07f, 1.3f);

    // Set the transformations (position and rotation) for book 5
    SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(-1.8f, 0.33f, 0.5f));

    // Apply the texture for book 5
    SetShaderTexture("cover2");
    SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("backdrop");

    // Draw book 5
    m_basicMeshes->DrawBoxMesh();

    /******************************************************************/
	// Draw Box (book 5 - pages)
	SetShaderColor(0.3f, 0.7f, 0.4f, 1.0f); // Light green color

	// Set the scale for book 5 pages
	scaleXYZ = glm::vec3(1.08f, 0.05f, 1.301f);

	// Set the transformations (position and rotation) for book 5 pages
	SetTransformations(scaleXYZ, 0.0f, 0.0f, 0.0f, glm::vec3(-1.789f, 0.33f, 0.5f));

	// Apply the texture for book 5 pages
	SetShaderTexture("pages"); 
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("wood");

	// Draw book 5 pages
	m_basicMeshes->DrawBoxMesh();

	/******************************************************************/
	// Draw Cylinder (pencil)
	SetShaderColor(1.0f, 0.85f, 0.6f, 1.0f); // Light wood color for the pencil body
	scaleXYZ = glm::vec3(0.03f, 1.0f, 0.03f); // Thin, long cylinder for the pencil body
	SetTransformations(scaleXYZ, 0.0f, -15.0f, -15.0f, glm::vec3(1.53f, 0.9f, 1.0f)); // Position it above the pencil holder at an angle

	SetShaderTexture("pencil");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("cover");

	m_basicMeshes->DrawCylinderMesh();

	// Draw Pencil Top
	scaleXYZ = glm::vec3(0.0299f, 1.0001f, 0.0299f); // Scale
	SetTransformations(scaleXYZ, 0.0f, -15.0f, -15.0f, glm::vec3(1.53f, 0.9f, 1.0f)); // Position

	SetShaderTexture("penciltop");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("cover");

	m_basicMeshes->DrawCylinderMesh();


	/******************************************************************/
	// Draw Cylinder (pencil 2)
	SetShaderColor(1.0f, 0.85f, 0.6f, 1.0f); // Light wood color for the pencil body
	scaleXYZ = glm::vec3(0.0301f, 1.0f, 0.03f); // Thin, long cylinder for the pencil body
	SetTransformations(scaleXYZ, 0.0f, -15.0f, -25.0f, glm::vec3(1.4f, 0.9f, 1.1f)); // Position e

	SetShaderTexture("pencil");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("cover");

	m_basicMeshes->DrawCylinderMesh();

	// Draw Pencil Top
	scaleXYZ = glm::vec3(0.0299f, 1.002f, 0.0299f); // Scale
	SetTransformations(scaleXYZ, 0.0f, -15.0f, -25.0f, glm::vec3(1.4f, 0.9f, 1.1f)); // Position 

	SetShaderTexture("penciltop");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("cover");

	m_basicMeshes->DrawCylinderMesh();

	/******************************************************************/
	// Draw Cylinder (pencil 3)
	SetShaderColor(1.0f, 0.85f, 0.6f, 1.0f); // Light wood color for the pencil body
	scaleXYZ = glm::vec3(0.03f, 1.0f, 0.03f); // Thin, long cylinder for the pencil body
	SetTransformations(scaleXYZ, 0.0f, -15.0f, -15.0f, glm::vec3(1.3f, 0.9f, 1.0f)); // Position

	SetShaderTexture("pencil");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("cover");

	m_basicMeshes->DrawCylinderMesh();

	// Draw Pencil Top
	scaleXYZ = glm::vec3(0.0299f, 1.001f, 0.0299f); // Scale
	SetTransformations(scaleXYZ, 0.0f, -15.0f, -15.0f, glm::vec3(1.3f, 0.9f, 1.0f)); // Position

	SetShaderTexture("penciltop");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("cover");

	m_basicMeshes->DrawCylinderMesh();

	/******************************************************************/
	// Draw Cylinder (pencil 4 - black)
	SetShaderColor(1.0f, 0.85f, 0.6f, 1.0f); // Light wood color for the pencil body
	scaleXYZ = glm::vec3(0.03f, 1.0f, 0.03f); // Thin, long cylinder for the pencil body
	SetTransformations(scaleXYZ, 15.0f, 0.0f, -15.0f, glm::vec3(1.35f, 0.9f, 1.11f)); // Position

	SetShaderTexture("pencil2");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("cover");

	m_basicMeshes->DrawCylinderMesh();

	// Draw Pencil Top
	scaleXYZ = glm::vec3(0.0299f, 1.001f, 0.0299f); // Scale
	SetTransformations(scaleXYZ, 15.0f, 0.0f, -15.0f, glm::vec3(1.35f, 0.9f, 1.11f)); // Position 

	SetShaderTexture("penciltop2");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("cover");

	m_basicMeshes->DrawCylinderMesh();

	/******************************************************************/
	// Draw Cylinder (pencil 5 - black)
	SetShaderColor(1.0f, 0.85f, 0.6f, 1.0f); // Light wood color for the pencil body
	scaleXYZ = glm::vec3(0.03f, 1.0f, 0.03f); // Thin, long cylinder for the pencil body
	SetTransformations(scaleXYZ, 10.0f, -15.0f, -5.0f, glm::vec3(1.2f, 0.9f, 1.13f)); // Position 

	SetShaderTexture("pencil2");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("cover");

	m_basicMeshes->DrawCylinderMesh();

	// Draw Pencil Top
	scaleXYZ = glm::vec3(0.0299f, 1.001f, 0.0299f); // Scale
	SetTransformations(scaleXYZ, 10.0f, -15.0f, -5.0f, glm::vec3(1.2f, 0.9f, 1.13f)); // Position 

	SetShaderTexture("penciltop2");
	SetTextureUVScale(1.0f, 1.0f);
	SetShaderMaterial("cover");

	m_basicMeshes->DrawCylinderMesh();

}



