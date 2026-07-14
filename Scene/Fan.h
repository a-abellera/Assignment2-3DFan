/**
 * @file Fan.h
 * @brief Implementation of the 3D Fan model.
 *
 * Part Hierarchy:
 * The fan is constructed from a single shared cube geometry rendered seven times
 * using a hierarchical Transform stack (push/pop).
 * - Base: Static, sits on the table plane.
 * - Pole: Static, connects the base to the hub.
 * - Hub: Static, serves as the pivot point for the blades.
 * - Blades (x4): Dynamic, rotate around the hub's Z-axis, separated by 90°.
 *
 * Gesture Behaviour:
 * - TAP (movement < kTapThreshold): Toggles the fan state (ON/OFF).
 * - SWIPE/DRAG: Temporarily increases rotation speed proportional to drag velocity.
 *   Speed returns to kBaseSpeed upon release.
 */

#ifndef FAN_H
#define FAN_H

#include "Model.h"
#include "Transform.h"

#include <chrono>
#include <glm/glm.hpp>

#ifdef PLATFORM_ANDROID
#include <android/asset_manager.h>
#endif

class Fan : public Model
{
public:
#ifdef PLATFORM_ANDROID
    // Construct with the Android asset manager, used to load shader source
    // from the APK's assets/ directory.
    explicit Fan( AAssetManager* mgr );
#else
    // Construct with no platform-specific state (desktop/web load shaders
    // directly from the filesystem / virtual FS).
    Fan( void );
#endif
    // Releases all GL resources via DeInitModel().
    ~Fan( void ) override;

    // Builds shaders and cube geometry, and initializes the transform stack.
    // Logs and returns early if shader compilation/linking fails.
    void InitModel( void ) override;

    // Deletes VAO/VBO/IBO/shader program and resets their handles to 0.
    // Not an override -- Model has no virtual DeInitModel().
    void DeInitModel( void );

    // Advances the blade spin angle, then draws base, pole, hub, and all
    // four blades for the current frame.
    void Render( void ) override;

    // Updates the GL viewport and recomputes the projection matrix for the
    // new aspect ratio.
    void Resize( int width, int height ) override;

    // Begins a touch/mouse gesture: records the start position and resets
    // the accumulated move distance used to distinguish tap vs. drag.
    void TouchEventDown( float x, float y ) override;

    // Continues a gesture: accumulates distance moved and updates dragBoost
    // based on pointer velocity since the last move event.
    void TouchEventMove( float x, float y ) override;

    // Ends a gesture: toggles fanOn if the total move distance was below
    // kTapThreshold (i.e. it was a tap, not a drag), and clears dragBoost.
    void TouchEventRelease( float x, float y ) override;

private:
    // Compiles and links the vertex/fragment shader program for this
    // platform (Android assets vs. filesystem/virtual FS).
    void BuildShaders( void );

    // Uploads the shared cube's positions, per-corner shades, and indices
    // into a VBO/IBO, and configures the VAO's vertex attribute pointers.
    void BuildCube( void );

    // Uploads the current model-view-projection matrix and part color
    // uniforms, then issues the draw call for one cube-shaped part.
    void DrawPart( const glm::vec3 &color );

    Transform transform;

    float spinAngle = 0.0f;
    bool  fanOn     = true;
    float dragBoost = 0.0f;

    static constexpr float kBaseSpeed    = 1.5f;
    static constexpr float kMaxBoost     = 20.0f;
    static constexpr float kBoostScale   = 8.0f;
    static constexpr float kTapThreshold = 12.0f;

    float lastX = 0.0f, lastY = 0.0f;
    float movedDistance = 0.0f;
    std::chrono::steady_clock::time_point lastMoveTime;

    GLuint vbo = 0;
    GLuint ibo = 0;
    GLuint vao = 0;
    GLuint shaderProgram = 0;
    GLint  mvpLoc   = -1;
    GLint  colorLoc = -1;

    float aspectRatio = 1.0f;

#ifdef PLATFORM_ANDROID
    AAssetManager* assetMgr = nullptr;
#endif

    static const glm::vec3 kBaseColor;
    static const glm::vec3 kPoleColor;
    static const glm::vec3 kHubColor;
    static const glm::vec3 kBladeColors[ 4 ];
};

#endif // FAN_H