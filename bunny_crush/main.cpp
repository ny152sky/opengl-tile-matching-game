#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <GL/glew.h>   // The GL Header File
#include <GL/gl.h>   // The GL Header File
#include <GLFW/glfw3.h> // The GLFW header
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H


#define BUFFER_OFFSET(i) ((char*)NULL + (i))

using namespace std;

//GLuint gProgram[3];
GLuint gProgram[6];
GLint gIntensityLoc;
float gIntensity = 1000;
int gWidth = 640, gHeight = 600;

struct Vertex
{
    Vertex(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Texture
{
    Texture(GLfloat inU, GLfloat inV) : u(inU), v(inV) { }
    GLfloat u, v;
};

struct Normal
{
    Normal(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Face
{
	Face(int v[], int t[], int n[]) {
		vIndex[0] = v[0];
		vIndex[1] = v[1];
		vIndex[2] = v[2];
		tIndex[0] = t[0];
		tIndex[1] = t[1];
		tIndex[2] = t[2];
		nIndex[0] = n[0];
		nIndex[1] = n[1];
		nIndex[2] = n[2];
	}
    GLuint vIndex[3], tIndex[3], nIndex[3];
};

struct Object
{
  Object() {
    color = glm::vec3(1, 0, 0);
    isPopped = 0;
    willSlide = 0;
    isPressed = 0;
    isMatched = 0;
    ySlide = 0;
  }
  glm::vec3 color;
  bool isPopped;
  bool willSlide;
  bool isPressed;
  bool isMatched;
  int ySlide;
};

vector<Vertex> gVertices;
vector<Texture> gTextures;
vector<Normal> gNormals;
vector<Face> gFaces;

GLuint gVertexAttribBuffer, gTextVBO, gIndexBuffer;
GLint gInVertexLoc, gInNormalLoc;
int gVertexDataSizeInBytes, gNormalDataSizeInBytes;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    GLuint TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
    GLuint Advance;    // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;


bool ParseObj(const string& fileName)
{
    fstream myfile;

    // Open the input
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            stringstream str(curLine);
            GLfloat c1, c2, c3;
            GLuint index[9];
            string tmp;

            if (curLine.length() >= 2)
            {
                if (curLine[0] == '#') // comment
                {
                    continue;
                }
                else if (curLine[0] == 'v')
                {
                    if (curLine[1] == 't') // texture
                    {
                        str >> tmp; // consume "vt"
                        str >> c1 >> c2;
                        gTextures.push_back(Texture(c1, c2));
                    }
                    else if (curLine[1] == 'n') // normal
                    {
                        str >> tmp; // consume "vn"
                        str >> c1 >> c2 >> c3;
                        gNormals.push_back(Normal(c1, c2, c3));
                    }
                    else // vertex
                    {
                        str >> tmp; // consume "v"
                        str >> c1 >> c2 >> c3;
                        gVertices.push_back(Vertex(c1, c2, c3));
                    }
                }
                else if (curLine[0] == 'f') // face
                {
                    str >> tmp; // consume "f"
					char c;
					int vIndex[3],  nIndex[3], tIndex[3];
					str >> vIndex[0]; str >> c >> c; // consume "//"
					str >> nIndex[0];
					str >> vIndex[1]; str >> c >> c; // consume "//"
					str >> nIndex[1];
					str >> vIndex[2]; str >> c >> c; // consume "//"
					str >> nIndex[2];

					assert(vIndex[0] == nIndex[0] &&
						   vIndex[1] == nIndex[1] &&
						   vIndex[2] == nIndex[2]); // a limitation for now

					// make indices start from 0
					for (int c = 0; c < 3; ++c)
					{
						vIndex[c] -= 1;
						nIndex[c] -= 1;
						tIndex[c] -= 1;
					}

                    gFaces.push_back(Face(vIndex, tIndex, nIndex));
                }
                else
                {
                    cout << "Ignoring unidentified line in obj file: " << curLine << endl;
                }
            }

            //data += curLine;
            if (!myfile.eof())
            {
                //data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }


	assert(gVertices.size() == gNormals.size());

    return true;
}

bool ReadDataFromFile(
    const string& fileName, ///< [in]  Name of the shader file
    string&       data)     ///< [out] The contents of the file
{
    fstream myfile;

    // Open the input
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            data += curLine;
            if (!myfile.eof())
            {
                data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

    return true;
}

void createVS(GLuint& program, const string& filename)
{
    string shaderSource;

    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &shader, &length);
    glCompileShader(vs);

    char output[1024] = {0};
    glGetShaderInfoLog(vs, 1024, &length, output);
    printf("VS compile log: %s\n", output);

    glAttachShader(program, vs);
}

void createFS(GLuint& program, const string& filename)
{
    string shaderSource;

    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &shader, &length);
    glCompileShader(fs);

    char output[1024] = {0};
    glGetShaderInfoLog(fs, 1024, &length, output);
    printf("FS compile log: %s\n", output);

    glAttachShader(program, fs);
}

void initShaders()
{
    gProgram[0] = glCreateProgram();

    gProgram[5] = glCreateProgram();

    createVS(gProgram[0], "vert0.glsl");
    createFS(gProgram[0], "frag0.glsl");



    createVS(gProgram[5], "vert_text.glsl");
    createFS(gProgram[5], "frag_text.glsl");

    glBindAttribLocation(gProgram[0], 0, "inVertex");
    glBindAttribLocation(gProgram[0], 1, "inNormal");

    glBindAttribLocation(gProgram[5], 2, "vertex");

    glLinkProgram(gProgram[0]);

    glLinkProgram(gProgram[5]);
    glUseProgram(gProgram[0]);

    gIntensityLoc = glGetUniformLocation(gProgram[0], "intensity");
    cout << "gIntensityLoc = " << gIntensityLoc << endl;
    glUniform1f(gIntensityLoc, gIntensity);
}

void initVBO()
{
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    assert(glGetError() == GL_NONE);

    glGenBuffers(1, &gVertexAttribBuffer);
    glGenBuffers(1, &gIndexBuffer);

    assert(gVertexAttribBuffer > 0 && gIndexBuffer > 0);

    glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

    gVertexDataSizeInBytes = gVertices.size() * 3 * sizeof(GLfloat);
    gNormalDataSizeInBytes = gNormals.size() * 3 * sizeof(GLfloat);
    int indexDataSizeInBytes = gFaces.size() * 3 * sizeof(GLuint);
    GLfloat* vertexData = new GLfloat [gVertices.size() * 3];
    GLfloat* normalData = new GLfloat [gNormals.size() * 3];
    GLuint* indexData = new GLuint [gFaces.size() * 3];

    float minX = 1e6, maxX = -1e6;
    float minY = 1e6, maxY = -1e6;
    float minZ = 1e6, maxZ = -1e6;

    for (int i = 0; i < gVertices.size(); ++i)
    {
        vertexData[3*i] = gVertices[i].x;
        vertexData[3*i+1] = gVertices[i].y;
        vertexData[3*i+2] = gVertices[i].z;

        minX = std::min(minX, gVertices[i].x);
        maxX = std::max(maxX, gVertices[i].x);
        minY = std::min(minY, gVertices[i].y);
        maxY = std::max(maxY, gVertices[i].y);
        minZ = std::min(minZ, gVertices[i].z);
        maxZ = std::max(maxZ, gVertices[i].z);
    }

    std::cout << "minX = " << minX << std::endl;
    std::cout << "maxX = " << maxX << std::endl;
    std::cout << "minY = " << minY << std::endl;
    std::cout << "maxY = " << maxY << std::endl;
    std::cout << "minZ = " << minZ << std::endl;
    std::cout << "maxZ = " << maxZ << std::endl;

    for (int i = 0; i < gNormals.size(); ++i)
    {
        normalData[3*i] = gNormals[i].x;
        normalData[3*i+1] = gNormals[i].y;
        normalData[3*i+2] = gNormals[i].z;
    }

    for (int i = 0; i < gFaces.size(); ++i)
    {
        indexData[3*i] = gFaces[i].vIndex[0];
        indexData[3*i+1] = gFaces[i].vIndex[1];
        indexData[3*i+2] = gFaces[i].vIndex[2];
    }


    glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes + gNormalDataSizeInBytes, 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, gVertexDataSizeInBytes, vertexData);
    glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes, gNormalDataSizeInBytes, normalData);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSizeInBytes, indexData, GL_STATIC_DRAW);

    // done copying; can free now
    delete[] vertexData;
    delete[] normalData;
    delete[] indexData;

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));

}

void initFonts(int windowWidth, int windowHeight)
{
    // Set OpenGL options
    //glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(windowWidth), 0.0f, static_cast<GLfloat>(windowHeight));
    glUseProgram(gProgram[5]);
    glUniformMatrix4fv(glGetUniformLocation(gProgram[5], "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // FreeType
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    }

    // Load font as face
    FT_Face face;
    if (FT_New_Face(ft, "/usr/share/fonts/truetype/liberation/LiberationSerif-Italic.ttf", 0, &face))
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    }

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Load first 128 characters of ASCII set
    for (GLubyte c = 0; c < 128; c++)
    {
        // Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
                );
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use
        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            face->glyph->advance.x
        };
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    //
    // Configure VBO for texture quads
    //
    glGenBuffers(1, &gTextVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void init(const char *input_file_name)
{
	//ParseObj("armadillo.obj");
	//ParseObj("bunny.obj");
  ParseObj(input_file_name);

    glEnable(GL_DEPTH_TEST);
    initShaders();
    initFonts(gWidth, gHeight);
    initVBO();
}

void drawModel()
{
	glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));

	glDrawElements(GL_TRIANGLES, gFaces.size() * 3, GL_UNSIGNED_INT, 0);
}

void renderText(const std::string& text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
    // Activate corresponding render state
    glUseProgram(gProgram[5]);
    glUniform3f(glGetUniformLocation(gProgram[5], "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        // Update VBO for each character
        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos,     ypos,       0.0, 1.0 },
            { xpos + w, ypos,       1.0, 1.0 },

            { xpos,     ypos + h,   0.0, 0.0 },
            { xpos + w, ypos,       1.0, 1.0 },
            { xpos + w, ypos + h,   1.0, 0.0 }
        };

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

        //glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)

        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

std::vector<std::vector<Object>> bunnies;
std::vector<Object> newBunnies;
std::vector<int> toDrop;
int gridcol, gridrow;
bool setColor = 0;
bool setNew = 0;
int moveCounter = 0;
int numOfMatched = 0;

void addBunnies(std::vector<Object> &newBunnies, int numOfBuns)
{
  srand(time(NULL));
  glm::vec3 colors[] = {glm::vec3(0, 0.8, 0.8), glm::vec3(1, 0.5, 0), glm::vec3(0, 0, 0.8), glm::vec3(1, 0, 0), glm::vec3(0.4, 0, 0.8)};

  int colorId;
  for (int i = 0; i < numOfBuns; i++)
  {
      colorId = (rand() % (5));
      if (setNew)
      {
        Object newBunny = Object();
        newBunny.color = colors[colorId];
        newBunnies.push_back(newBunny);
      }
  }
  setNew = 0;
}

void randomColors(std::vector<std::vector<Object>> &bunnies)
{
  srand(time(NULL));
  glm::vec3 colors[] = {glm::vec3(0, 0.8, 0.8), glm::vec3(1, 0.5, 0), glm::vec3(0, 0, 0.8), glm::vec3(1, 0, 0), glm::vec3(0.4, 0, 0.8)};

  int colorId;
  for (int i = 0; i < gridrow; i++)
  {
    for (int j = 0; j < gridcol; j++)
    {
      colorId = (rand() % (5));
      if (setColor)
      {
        bunnies[i][j] = Object();
        bunnies[i][j].color = colors[colorId];
      }
    }
  }
  setColor = 0;
}

bool pressed = 0;
int pressrow = -1;
int presscol = -1;
int sliderow, slidecol;
int EVENT = 0;

//int gridcol, gridrow;

void normalDraw(std::vector<std::vector<Object>> &bunnies, float angle, int i, int j)
{
  glUseProgram(gProgram[0]);

  glm::vec3 bunnycolor = bunnies[i][j].color;

  float gridX = 20/float(gridcol);
  float gridY = 19/float(gridrow);

  float scaling2 = 30.f;

  glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(scaling2/(gridrow*gridcol), scaling2/(gridrow*gridcol), scaling2/(gridrow*gridcol)));
  glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(-10.f + j * (gridX)+ gridX/2, 10.f - (i * (gridY) + gridY/2), -10.f));
  glm::mat4 R = glm::rotate(glm::mat4(1.f), glm::radians(angle), glm::vec3(0, 1, 0));
  glm::mat4 modelMat = T * R * S;
  glm::mat4 modelMatInv = glm::transpose(glm::inverse(modelMat));
  glm::mat4 orthoMat = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 20.0f);

  glUniform3f(glGetUniformLocation(gProgram[0], "kd"), bunnycolor.x, bunnycolor.y, bunnycolor.z);
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "orthoMat"), 1, GL_FALSE, glm::value_ptr(orthoMat));

  drawModel();
}

void pop(std::vector<std::vector<Object>> &bunnies, float angle, int i, int j, float& scaling)
{
  glUseProgram[0];

  glm::vec3 bunnycolor = bunnies[i][j].color;

  float gridX = 20/float(gridcol);
  float gridY = 19/float(gridrow);

  float scaling2 = 30.f;

  glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(scaling*(scaling2/(gridrow*gridcol)), scaling*(scaling2/(gridrow*gridcol)), scaling*(scaling2/(gridrow*gridcol))));
  glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(-10.f + j * (gridX)+ gridX/2, 10.f - (i * (gridY) + gridY/2), -10.f));
  glm::mat4 R = glm::rotate(glm::mat4(1.f), glm::radians(angle), glm::vec3(0, 1, 0));
  glm::mat4 modelMat = T * R * S;
  glm::mat4 modelMatInv = glm::transpose(glm::inverse(modelMat));
  glm::mat4 orthoMat = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 20.0f);

  glUniform3f(glGetUniformLocation(gProgram[0], "kd"), bunnycolor.x, bunnycolor.y, bunnycolor.z);
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "orthoMat"), 1, GL_FALSE, glm::value_ptr(orthoMat));

  if(scaling <= 1.5)
    drawModel();
  else
  {

    bunnies[i][j].isPressed = 0;
    bunnies[i][j].isPopped = 1;
    for (int a = i-1; a >= 0; a--)
    {
      bunnies[a][j].willSlide = 1;
    }

    //sliderow = pressrow-1;
    //slidecol = presscol;
    //presscol = -1;
    //pressrow = -1;
    scaling = 1.01;


      EVENT = 2;
  }
  scaling += 0.01;
}

void pop2(std::vector<std::vector<Object>> &bunnies, float angle, int i, int j, float& scaling)
{
  glUseProgram[0];

  glm::vec3 bunnycolor = bunnies[i][j].color;

  float gridX = 20/float(gridcol);
  float gridY = 19/float(gridrow);

  float scaling2 = 30.f;

  glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(scaling*(scaling2/(gridrow*gridcol)), scaling*(scaling2/(gridrow*gridcol)), scaling*(scaling2/(gridrow*gridcol))));
  glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(-10.f + j * (gridX)+ gridX/2, 10.f - (i * (gridY) + gridY/2), -10.f));
  glm::mat4 R = glm::rotate(glm::mat4(1.f), glm::radians(angle), glm::vec3(0, 1, 0));
  glm::mat4 modelMat = T * R * S;
  glm::mat4 modelMatInv = glm::transpose(glm::inverse(modelMat));
  glm::mat4 orthoMat = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 20.0f);

  glUniform3f(glGetUniformLocation(gProgram[0], "kd"), bunnycolor.x, bunnycolor.y, bunnycolor.z);
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "orthoMat"), 1, GL_FALSE, glm::value_ptr(orthoMat));

  if(scaling <= 1.5)
  {
    drawModel();
    scaling += 0.01;
  }
  else
  {
    scaling = 1.01;
    for (int a = 0; a < gridrow; a++)
    {
      for (int b  = 0; b < gridcol; b++)
      {
        if (bunnies[a][b].isMatched)
        {
          bunnies[a][b].isMatched = 0;
          bunnies[a][b].isPopped = 1;
        }
      }
    }
    setNew = 1;
    addBunnies(newBunnies,1);
    bunnies[i][j].color = newBunnies.back().color;
    newBunnies.pop_back();

    for (int a = i-1; a >= 0; a--)
    {
      bunnies[a][j].willSlide = 1;
    }

    //sliderow = pressrow-1;
    //slidecol = presscol;
    //presscol = -1;
    //pressrow = -1;
    EVENT = 0;
  }

}

void dropNewBunny(std::vector<std::vector<Object>> &bunnies, float angle, float& slide, int sliderow, int slidecol)
{
  glUseProgram[0];

  //glm::vec3 bunnycolor = glm::vec3(0, 1, 0);
  glm::vec3 bunnycolor = newBunnies.back().color;

  float gridX = 20/float(gridcol);
  float gridY = 19/float(gridrow);

  float scaling2 = 30.f;

  glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(scaling2/(gridrow*gridcol), scaling2/(gridrow*gridcol), scaling2/(gridrow*gridcol)));
  glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(-10.f + slidecol * (gridX)+ gridX/2, 10.f - ((-1) * (gridY) + gridY/2) - slide, -10.f));
  glm::mat4 R = glm::rotate(glm::mat4(1.f), glm::radians(angle), glm::vec3(0, 1, 0));
  glm::mat4 modelMat = T * R * S;
  glm::mat4 modelMatInv = glm::transpose(glm::inverse(modelMat));
  glm::mat4 orthoMat = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 20.0f);

  glUniform3f(glGetUniformLocation(gProgram[0], "kd"), bunnycolor.x, bunnycolor.y, bunnycolor.z);
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "orthoMat"), 1, GL_FALSE, glm::value_ptr(orthoMat));

  if (slide < gridY)
  {
    drawModel();
    //slide += 0.05;
  }
  else
  {
    slide = 0.05;

  }
}

void drop(std::vector<std::vector<Object>> &bunnies, float angle, int i, int j, float& slide, int sliderow, int slidecol)
{
  glUseProgram[0];

  glm::vec3 bunnycolor = bunnies[i][j].color;

  float gridX = 20/float(gridcol);
  float gridY = 19/float(gridrow);

  float scaling2 = 30.f;

  glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(scaling2/(gridrow*gridcol), scaling2/(gridrow*gridcol), scaling2/(gridrow*gridcol)));
  glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(-10.f + j * (gridX)+ gridX/2, 10.f - (i * (gridY) + gridY/2) - slide, -10.f));
  glm::mat4 R = glm::rotate(glm::mat4(1.f), glm::radians(angle), glm::vec3(0, 1, 0));
  glm::mat4 modelMat = T * R * S;
  glm::mat4 modelMatInv = glm::transpose(glm::inverse(modelMat));
  glm::mat4 orthoMat = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 20.0f);

  glUniform3f(glGetUniformLocation(gProgram[0], "kd"), bunnycolor.x, bunnycolor.y, bunnycolor.z);
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "orthoMat"), 1, GL_FALSE, glm::value_ptr(orthoMat));

  if (slide < gridY)
  {
    drawModel();
    dropNewBunny(bunnies, angle, slide, sliderow, slidecol);
    slide += 0.05;
  }
  else
  {
    slide = 0.05;

    for(int a = sliderow; a >= 0; a--)
    {
      bunnies[a+1][j].color = bunnies[a][j].color;
      bunnies[a][j].willSlide = 0;
      bunnies[a][j].isPopped = 0;
    }

    //bunnies[0][slidecol].color = glm::vec3(0, 1, 0);
    bunnies[0][slidecol].color = newBunnies.back().color;

    bunnies[sliderow+1][slidecol].isPopped = 0;
    EVENT = 3;
  }
}

void drop2(std::vector<std::vector<Object>> &bunnies, float angle, int i, int j, float& slide)
{
  glUseProgram[0];

  glm::vec3 bunnycolor = bunnies[i][j].color;

  float gridX = 20/float(gridcol);
  float gridY = 19/float(gridrow);

  float scaling2 = 30.f;

  glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(scaling2/(gridrow*gridcol), scaling2/(gridrow*gridcol), scaling2/(gridrow*gridcol)));
  glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(-10.f + j * (gridX)+ gridX/2, 10.f - (i * (gridY) + gridY/2) - slide, -10.f));
  glm::mat4 R = glm::rotate(glm::mat4(1.f), glm::radians(angle), glm::vec3(0, 1, 0));
  glm::mat4 modelMat = T * R * S;
  glm::mat4 modelMatInv = glm::transpose(glm::inverse(modelMat));
  glm::mat4 orthoMat = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -20.0f, 20.0f);

  glUniform3f(glGetUniformLocation(gProgram[0], "kd"), bunnycolor.x, bunnycolor.y, bunnycolor.z);
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
  glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "orthoMat"), 1, GL_FALSE, glm::value_ptr(orthoMat));

  if (slide < gridY)
  {
    drawModel();
    dropNewBunny(bunnies, angle, slide, sliderow, slidecol);
    slide += 0.05;
  }
  else
  {
    slide = 0.05;

    for(int a = sliderow; a >= 0; a--)
    {
      bunnies[a+1][j].color = bunnies[a][j].color;
      bunnies[a][j].willSlide = 0;
      bunnies[a][j].isPopped = 0;
    }

    //bunnies[0][slidecol].color = glm::vec3(0, 1, 0);
    bunnies[0][slidecol].color = newBunnies.back().color;

    bunnies[sliderow+1][slidecol].isPopped = 0;
    EVENT = 3;
  }
}

void matchAndPop(std::vector<std::vector<Object>> &bunnies, float angle, float& scaling, bool matched)
{
  for (int i = 0; i < gridrow; i++)
  {
    for (int j = 0; j < gridcol; j++)
    {
      if (!bunnies[i][j].isMatched)
      {
        if (i < (gridrow - 2) && bunnies[i][j].color == bunnies[i+1][j].color && bunnies[i][j].color == bunnies[i+2][j].color)
        {
          int a = 3;
          while ( (i+a) < (gridrow) && bunnies[i][j].color == bunnies[i+a][j].color)
          {
            a++;
          }
          for (int x = i; x < (i+a); x++)
          {
            bunnies[x][j].isMatched = 1;
            numOfMatched++;
          }
        }
        if (j < (gridcol - 2) && bunnies[i][j].color == bunnies[i][j+1].color && bunnies[i][j].color == bunnies[i][j+2].color)
        {
          int b = 3;
          while ( (j+b) < (gridcol) && bunnies[i][j].color == bunnies[i][j+b].color)
          {
            b++;
          }
          for (int x = j; x < (j+b); x++)
          {
            bunnies[i][x].isMatched = 1;
            numOfMatched++;
          }
        }
      }

    }
  }

  for (int i = 0; i < gridrow; i++)
  {
    for (int j = 0; j < gridcol; j++)
    {
      if (bunnies[i][j].isMatched)
      {
        toDrop[j]++;
      }
    }
  }

  EVENT = 4;
}



void display(std::vector<std::vector<int>> &colorIds, std::vector<std::vector<Object>> &bunnies)
{
    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	  static float angle = 0;
    static float scaling = 1.01;
    static float slide = 0.05;
    static bool matched = 0;

    randomColors(bunnies);

    if (EVENT == 0)
    {
      for(int i = 0; i < gridrow ; i++)
      {
        for(int j = 0; j < gridcol ; j++)
        {
            bunnies[i][j].isPopped = 0;
            bunnies[i][j].willSlide = 0;
            bunnies[i][j].isMatched = 0;


            normalDraw(bunnies, angle, i, j);
        }
      }
    }

    if (EVENT == 1)
    {
      if (pressrow != -1 && presscol != -1)
      {
        bunnies[pressrow][presscol].isPressed = 1;
      }
      for(int i = 0; i < gridrow ; i++)
      {
        for(int j = 0; j < gridcol ; j++)
        {
            if (!bunnies[i][j].isPressed && !bunnies[i][j].isPopped)
              normalDraw(bunnies, angle, i, j);
            if (bunnies[i][j].isPressed && !bunnies[i][j].isPopped)
              pop(bunnies, angle, i, j, scaling);

        }
      }

    }

    if (EVENT == 2)
    {
      sliderow = pressrow-1;
      slidecol = presscol;

      setNew = 1;
      addBunnies(newBunnies, 1);

      for(int i = 0; i < gridrow ; i++)
      {
        for(int j = 0; j < gridcol ; j++)
        {
          if (!bunnies[i][j].willSlide && !bunnies[i][j].isPopped)
            normalDraw(bunnies, angle, i, j);
          if (bunnies[i][j].willSlide)
            drop(bunnies, angle, i, j, slide, sliderow, slidecol);
          if (bunnies[i][j].isPopped && pressrow == 0)
          {
            //normalDraw(bunnies, angle, i, j);
            bunnies[i][j].isPopped = 0;
            EVENT = 0;
          }
        }
      }
      //dropNewBunny(bunnies, angle, slide, sliderow, slidecol);
      newBunnies.pop_back();
    }

    if (EVENT == 3)
    {
      matchAndPop(bunnies, angle, scaling, matched);
        //EVENT = 0;
    }

    if (EVENT == 4)
    {
      int theresmatch = 0;
      for(int i = 0; i < gridrow ; i++)
      {
        for(int j = 0; j < gridcol ; j++)
        {
          if (!bunnies[i][j].isMatched && !bunnies[i][j].isPopped)
            normalDraw(bunnies, angle, i, j);
          else if (bunnies[i][j].isMatched && !bunnies[i][j].isPopped)
          {
            theresmatch++;
            pop2(bunnies, angle, i, j, scaling);
          }
        }
      }
      //cout << "MATCHED BUNNIES " << numOfMatched << endl;
      if (theresmatch == 0)
        EVENT = 0;
    }

    if (EVENT == 5)
    {
      /*
      for(int i = 0; i < gridrow ; i++)
      {
        for(int j = 0; j < gridcol ; j++)
        {
          if (!bunnies[i][j].isPopped)
          {
            for (int a = i+1; a < gridrow; i++)
            {
              if (bunnies[a][j].isPopped)
              {
                bunnies[i][j].ySlide++;
                bunnies[i][j].willSlide = 1;
              }
            }
            if (bunnies[i][j].ySlide == 0)
              bunnies[i][j].willSlide = 0;
          }
        }
      }
      */

      for(int i = 0; i < gridrow ; i++)
      {
        for(int j = 0; j < gridcol ; j++)
        {
          if (!bunnies[i][j].willSlide && !bunnies[i][j].isPopped)
            normalDraw(bunnies, angle, i, j);
          //if (toDrop[j] != 0)
            //drop(bunnies, angle, i, j, slide, sliderow, slidecol);
        }
      }
    }

    //assert(glGetError() == GL_NO_ERROR);

    std::string moveCount = std::to_string(moveCounter);
    std::string score = std::to_string(numOfMatched);


    std::string text = "Moves: " + moveCount + " Score: " + score;

    renderText(text, 0, 0, 1, glm::vec3(0, 1, 1));

    //assert(glGetError() == GL_NO_ERROR);

	angle += 0.5;
}


void reshape(GLFWwindow* window, int w, int h)
{
    w = w < 1 ? 1 : w;
    h = h < 1 ? 1 : h;

    gWidth = w;
    gHeight = h;

    glViewport(0, 0, w, h);
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    else if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        setColor = 1;
        randomColors(bunnies);
        EVENT = 0;
        moveCounter = 0;
        numOfMatched = 0;
    }
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    // grid size
    float gridX = gWidth/gridcol;
    float gridY = (gHeight-60)/gridrow;

    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        cout << "MOUSE PRESSED" << endl;
        double x, y;
        glfwGetCursorPos(window, &x, &y);
        cout << "Cursor Position at (" << x << " : " << y << endl;
        int a = x/gridX;
        int b = y/gridY;
        cout << "BUNNY NUMBER " << b << " by " << a << endl;
        if (EVENT == 0)
        {
          pressrow = b;
          presscol = a;
          EVENT = 1;
          moveCounter++;
        }
    }
}



void mainLoop(GLFWwindow* window)
{
  glm::vec3 colors[] = {glm::vec3(0, 0.8, 0.8), glm::vec3(1, 0.5, 0), glm::vec3(0, 0, 0.8), glm::vec3(1, 0, 0), glm::vec3(0.4, 0, 0.8)};

  std::vector<std::vector<int>> colorIds;
  colorIds.resize(gridrow, vector<int>(gridcol, 0));

//  std::vector<std::vector<Object>> bunnies;
  bunnies.resize(gridrow, vector<Object>(gridcol, Object()));
  toDrop.resize(gridcol, 0);

  setColor = 1;

    while (!glfwWindowShouldClose(window))
    {
        display(colorIds, bunnies);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

int main(int argc, char** argv)   // Create Main Function For Bringing It All Together
{
    if (argc != 4)
    {
        cout << "Please run the program as:" << endl
             << "\t./main <grid_width> <grid_height> <input_file_name>" << endl;
        return 1;
    }

    else
    {
        const char *w = argv[1];
        sscanf(w, "%d", &gridcol);

        const char *h = argv[2];
        sscanf(h, "%d", &gridrow);

        const char *input_file_name = argv[3];

        GLFWwindow* window;
        if (!glfwInit())
        {
            exit(-1);
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
        //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

        window = glfwCreateWindow(gWidth, gHeight, "Simple Example", NULL, NULL);

        if (!window)
        {
            glfwTerminate();
            exit(-1);
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        // Initialize GLEW to setup the OpenGL Function pointers
        if (GLEW_OK != glewInit())
        {
            std::cout << "Failed to initialize GLEW" << std::endl;
            return EXIT_FAILURE;
        }

        char rendererInfo[512] = {0};
        strcpy(rendererInfo, (const char*) glGetString(GL_RENDERER));
        strcat(rendererInfo, " - ");
        strcat(rendererInfo, (const char*) glGetString(GL_VERSION));
        glfwSetWindowTitle(window, rendererInfo);

        init(input_file_name);

        glfwSetKeyCallback(window, keyboard);
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwSetWindowSizeCallback(window, reshape);

        reshape(window, gWidth, gHeight); // need to call this once ourselves
        mainLoop(window); // this does not return unless the window is closed

        glfwDestroyWindow(window);
        glfwTerminate();

        return 0;
    }
}
