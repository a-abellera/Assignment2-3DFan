/**
 * @file Fan.cpp
 * @brief Logic for 3D Fan rendering, animation, and touch interaction.
 *
 * This implementation uses a single VAO/VBO for geometry and per-part colour
 * uploaded via the PARTCOLOR uniform.
 *
 * Hierarchy & Transform Logic:
 * Each part is wrapped in TransformPushMatrix() and TransformPopMatrix() to
 * isolate local transformations (Translate/Rotate/Scale). Transformations
 * are applied in parent-to-child order (post-multiplied).
 *
 * Gesture Logic:
 * Touch events are tracked via (lastX, lastY) and std::chrono to calculate
 * movement distance (for tap detection) and velocity (for drag boost).
 */

#include "Fan.h"
#include "ShaderHelper.h"

#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <algorithm>

namespace
{
    // ---- one 8-vertex cube, shared by every part -------------------------
    const GLfloat kCubePositions[ 8 * 3 ] =
            {
                    -0.5f, -0.5f,  0.5f,   // 0 front-bottom-left
                    0.5f, -0.5f,  0.5f,   // 1 front-bottom-right
                    0.5f,  0.5f,  0.5f,   // 2 front-top-right
                    -0.5f,  0.5f,  0.5f,   // 3 front-top-left
                    -0.5f, -0.5f, -0.5f,   // 4 back-bottom-left
                    0.5f, -0.5f, -0.5f,   // 5 back-bottom-right
                    0.5f,  0.5f, -0.5f,   // 6 back-top-right
                    -0.5f,  0.5f, -0.5f,   // 7 back-top-left
            };

    const GLfloat kCubeShades[ 8 ] =
            {
                    1.0f, 1.0f, 1.0f, 1.0f,   // front face
                    0.6f, 0.6f, 0.6f, 0.6f,   // back face
            };

    const GLushort kCubeIndices[ 36 ] =
            {
                    0, 1, 2,   2, 3, 0,   // front
                    1, 5, 6,   6, 2, 1,   // right
                    5, 4, 7,   7, 6, 5,   // back
                    4, 0, 3,   3, 7, 4,   // left
                    3, 2, 6,   6, 7, 3,   // top
                    4, 5, 1,   1, 0, 4,   // bottom
            };

#if defined(__ANDROID__)
    constexpr const char *kVertexShaderPath   = "shader/FanVertex.glsl";
    constexpr const char *kFragmentShaderPath = "shader/FanFragment.glsl";
#else
    constexpr const char *kVertexShaderPath   = "FanVertex.glsl";
    constexpr const char *kFragmentShaderPath = "FanFragment.glsl";
#endif
}

const glm::vec3 Fan::kBaseColor = glm::vec3( 0.45f, 0.28f, 0.12f );
const glm::vec3 Fan::kPoleColor = glm::vec3( 0.55f, 0.55f, 0.58f );
const glm::vec3 Fan::kHubColor  = glm::vec3( 0.20f, 0.20f, 0.22f );
const glm::vec3 Fan::kBladeColors[ 4 ] =
        {
                glm::vec3( 0.85f, 0.10f, 0.10f ),   // red
                glm::vec3( 0.10f, 0.35f, 0.85f ),   // blue
                glm::vec3( 0.95f, 0.55f, 0.10f ),   // orange
                glm::vec3( 0.20f, 0.75f, 0.25f ),   // green
        };

// -----------------------------------------------------------------------------
// Construction / destruction
// -----------------------------------------------------------------------------

#ifdef PLATFORM_ANDROID
Fan::Fan( AAssetManager* mgr ) : assetMgr( mgr )
{
}
#else
Fan::Fan( void )
{
}
#endif

Fan::~Fan( void )
{
    DeInitModel();
}

// -----------------------------------------------------------------------------
// Part 1 -- static geometry: one VBO (positions + shades as two
// sub-regions), one IBO, one VAO, one shader program.
// -----------------------------------------------------------------------------

// Builds the shader program and cube geometry, and initializes the
// transform stack. Logs an error and returns early if shader setup fails.
void Fan::InitModel( void )
{
    BuildShaders();
    if ( !shaderProgram )
    {
        LOGE( "Fan: shader program failed" );
        return;
    }

    mvpLoc   = glGetUniformLocation( shaderProgram, "MODELVIEWPROJECTIONMATRIX" );
    colorLoc = glGetUniformLocation( shaderProgram, "PARTCOLOR" );

    BuildCube();

    transform.TransformInit();
    lastMoveTime = std::chrono::steady_clock::now();
}

// Compiles and links the vertex/fragment shaders, dispatching to the
// Android-asset loader or the filesystem/virtual-FS loader as appropriate.
void Fan::BuildShaders( void )
{
#ifdef PLATFORM_ANDROID
    shaderProgram = ShaderHelper::buildProgramFromAssets( assetMgr, kVertexShaderPath, kFragmentShaderPath );
#else
    shaderProgram = ShaderHelper::buildProgramFromFile( kVertexShaderPath, kFragmentShaderPath );
#endif
}

// Uploads the shared cube's positions and per-corner shades into a single
// VBO (as two sub-regions), uploads the index buffer, and wires both
// vertex attributes into the VAO for later use in DrawPart().
void Fan::BuildCube( void )
{
    glGenVertexArrays( 1, &vao );
    glBindVertexArray( vao );

    glGenBuffers( 1, &vbo );
    glBindBuffer( GL_ARRAY_BUFFER, vbo );
    glBufferData( GL_ARRAY_BUFFER, sizeof( kCubePositions ) + sizeof( kCubeShades ), nullptr, GL_STATIC_DRAW );
    glBufferSubData( GL_ARRAY_BUFFER, 0,                        sizeof( kCubePositions ), kCubePositions );
    glBufferSubData( GL_ARRAY_BUFFER, sizeof( kCubePositions ), sizeof( kCubeShades ),    kCubeShades );

    // layout(location = 0) in vec3 POSITION;
    glEnableVertexAttribArray( 0 );
    glVertexAttribPointer( 0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void *>( 0 ) );

    // layout(location = 1) in float SHADE;
    glEnableVertexAttribArray( 1 );
    glVertexAttribPointer( 1, 1, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void *>( sizeof( kCubePositions ) ) );

    glGenBuffers( 1, &ibo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo );
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, sizeof( kCubeIndices ), kCubeIndices, GL_STATIC_DRAW );

    glBindVertexArray( 0 );
}

// Deletes the VAO, VBO, IBO, and shader program (if allocated) and resets
// all handles to 0 so this is safe to call multiple times.
void Fan::DeInitModel( void )
{
    if ( vao )           glDeleteVertexArrays( 1, &vao );
    if ( vbo )            glDeleteBuffers( 1, &vbo );
    if ( ibo )            glDeleteBuffers( 1, &ibo );
    if ( shaderProgram )  glDeleteProgram( shaderProgram );

    vao = vbo = ibo = shaderProgram = 0;
}

// Updates the GL viewport to the new size and rebuilds the projection
// matrix using the new aspect ratio.
void Fan::Resize( int width, int height )
{
    glViewport( 0, 0, width, height );
    aspectRatio = ( height > 0 ) ? static_cast<float>( width ) / static_cast<float>( height ) : 1.0f;

    transform.TransformSetMatrixMode( PROJECTION_MATRIX );
    transform.TransformLoadIdentity();
    transform.TransformSetPerspective( glm::radians( 60.0f ), aspectRatio, 0.01f, 1000.0f, 0.0f );
}

// -----------------------------------------------------------------------------
// Part 1 (draw) + Part 2 (animate) -- seven draw calls per frame, each with
// its own model matrix (Transform push/pop) and its own PARTCOLOR uniform.
// -----------------------------------------------------------------------------

// Uploads the current model-view-projection matrix and this part's color,
// then draws the shared cube mesh using the currently bound transform.
void Fan::DrawPart( const glm::vec3 &color )
{
    glm::mat4 mvp = *transform.TransformGetModelViewProjectionMatrix();

    glUniformMatrix4fv( mvpLoc, 1, GL_FALSE, glm::value_ptr( mvp ) );
    glUniform3fv( colorLoc, 1, glm::value_ptr( color ) );

    glBindVertexArray( vao );
    glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, reinterpret_cast<void *>( 0 ) );
    glBindVertexArray( 0 );
}

// Advances the blade spin angle (if the fan is on), clears the frame, then
// draws the base, pole, hub, and all four blades with their own transforms.
void Fan::Render( void )
{
    // ---- Part 2: advance the blade angle -----------------------------
    if ( fanOn )
        spinAngle += kBaseSpeed + dragBoost;
    if ( spinAngle >= 360.0f )
        spinAngle -= 360.0f;

    glEnable( GL_DEPTH_TEST );
    glUseProgram( shaderProgram );

    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    transform.TransformSetMatrixMode( MODEL_MATRIX );
    transform.TransformLoadIdentity();

    // world: shared pull-back + 3D tilt, applied before every part so the
    // whole fan reads as three-dimensional.
    transform.TransformTranslate( 0.0f, 0.8f, -8.0f );
    transform.TransformRotate( glm::radians( 20.0f ), 0.0f, 1.0f, 0.0f );

    // base -- sits on the "table"
    transform.TransformPushMatrix();
    transform.TransformTranslate( 0.0f, -2.6f, 0.0f );
    transform.TransformScale( 1.6f, 0.25f, 0.8f );
    DrawPart( kBaseColor );
    transform.TransformPopMatrix();

    // pole -- connects hub to base
    transform.TransformPushMatrix();
    transform.TransformTranslate( 0.0f, -1.21f, 0.0f );
    transform.TransformScale( 0.15f, 2.53f, 0.15f );
    DrawPart( kPoleColor );
    transform.TransformPopMatrix();

    // hub -- pivot the blades rotate around
    transform.TransformPushMatrix();
    transform.TransformTranslate( 0.0f, 0.2f, 0.0f );
    transform.TransformScale( 0.3f, 0.3f, 0.3f );
    DrawPart( kHubColor );
    transform.TransformPopMatrix();

    // four blades, 90 degrees apart, chasing spinAngle around the hub's Z axis
    for ( int i = 0; i < 4; ++i )
    {
        transform.TransformPushMatrix();
        transform.TransformTranslate( 0.0f, 0.2f, 0.15f );
        transform.TransformRotate( glm::radians( spinAngle + i * 90.0f ), 0.0f, 0.0f, 1.0f );
        transform.TransformTranslate( 0.0f, 0.55f, 0.0f );
        transform.TransformScale( 0.22f, 0.8f, 0.05f );
        DrawPart( kBladeColors[ i ] );
        transform.TransformPopMatrix();
    }
}

// -----------------------------------------------------------------------------
// Part 3 -- tap toggles fanOn.  Part 4 -- drag speed drives dragBoost.
// A single gesture pipeline (down -> move* -> release) does both, split by
// whether the finger/mouse moved more or less than kTapThreshold pixels.
// -----------------------------------------------------------------------------

// Records the gesture's starting position and resets the accumulated
// move distance used later to distinguish a tap from a drag.
void Fan::TouchEventDown( float x, float y )
{
    lastX = x;
    lastY = y;
    movedDistance = 0.0f;
    dragBoost = 0.0f;
    lastMoveTime  = std::chrono::steady_clock::now();
}

// Accumulates the distance moved since the last event and updates
// dragBoost based on the pointer's velocity over that interval.
void Fan::TouchEventMove( float x, float y )
{
    const float dx = x - lastX;
    const float dy = y - lastY;
    const float dist = std::sqrt(dx * dx + dy * dy);
    movedDistance += dist;

    const auto now = std::chrono::steady_clock::now();
    const float dtMs = std::chrono::duration<float, std::milli>(now - lastMoveTime).count();

    if (dtMs > 0.0f) {
        const float velocity = dist / dtMs;
        dragBoost = std::min(velocity * kBoostScale, kMaxBoost);
        LOGI("Fan drag velocity %.3f px/ms", velocity);
    }

    lastX = x;
    lastY = y;
    lastMoveTime = now;
}

// Ends the gesture: toggles fanOn if total movement stayed below
// kTapThreshold (a tap rather than a drag), and always clears dragBoost.
void Fan::TouchEventRelease( float x, float y )
{
    (void)x;
    (void)y;

    if ( movedDistance < kTapThreshold )
    {
        fanOn = !fanOn;
        LOGI( "Fan %s", fanOn ? "ON" : "OFF" );
    }

    dragBoost = 0.0f;
}
