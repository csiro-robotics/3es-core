
/*!
@mainpage 3rd Eye Scene Documentation

3<sup>rd</sup> Eye Scene is an instrumentation based visual debugger and debugging aid in the vein
of <a href="http://wiki.ros.org/rviz">rviz</a> or physics engine viewers such as <a
href="https://www.havok.com/physics/">Havok Visual Debugger</a> or <a
href="https://developer.nvidia.com/physx-visual-debugger">PhysX Visual Debugger</a>. Whereas those
tools are tightly bound to their respective SDKs, 3<sup>rd</sup> Eye Scene can be used to remotely
visualise and debug any real time or non real time 3D algorithm. Conceptually, it can be thought of
as a remote rendering application. A 3es server may be embedded into any program, then 3es render
commands may be interspersed throughout that program. The 3es viewer client application is then used
to view, record and playback these render commands.

This documentation covers the C++ API. Documentation is also available for the <a
href="csharp/index.html">C# API documentation</a> which includes the client viewer API.

@image html images/tes-anim.gif "Real Time Visualisation"

@image html images/usecase/sphere-anim.gif "Geometry Visualisation"

@section secfeatures Features

- Remote 3D rendering from any application
- Record rendered data
- Playback and step through recorded data
- Open, extensible protocol
- Plugin extensible to visualise specialised geometry

@section secusecase Use Cases

- Visualising geometric algorithms
  - Mesh operations
  - Geometric intersection tests
  - Point cloud processing
- Remote visualisation
  - Visualise 3D data from headless processes
- Real time visualisation
  - Remote visualisation
  - Visualise "hidden" data
    - Physics geometry
    - AI logic and constructs
- QA testing
  - Record test sessions and attach 3es files to bug reports.

@section secpages Other Information

- @subpage docviewer
- @subpage docorganisation
- @subpage docbuild
- @subpage docusage
- @subpage docprotocol
- @subpage docfileformat
- @subpage docplugins
- @subpage docthirdparty
- @subpage doclicense

*/
