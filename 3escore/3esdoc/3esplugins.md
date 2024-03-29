# Message handler plugins {#docplugins}

> This page is for the C# API. The C++ API has yet to support plugins.

The 3<sup>rd</sup> Eye Scene protocol is extensible and allows for user defined messages. Writing 3<sup>rd</sup> Eye Scene handles for user messages requires authoring of a viewer plugin. Viewer plugins are .Net DLLs which include derivations of the `Tes.Runtime.MessageHandler` for dealing with user messages.

Most `Tes.Runtime.MessageHandler` implementations deal with 3D objects; primitives, meshes and text. However, some message handlers deal with other types of data. The `Tes.Handlers.CategoriesHandler` for example, manages the names of and links between categories and has 3D representation. User messages may follow either pattern, depending on the need.

For example, in games, user messages may be used to visualise an agent's game attributes in a single message. Health, primary AI state, ammunition, etc. may be collated into a much smaller single message as opposed to using  multiple text messages.

## Authoring a 3rd Eye Scene plugin

To author a 3<sup>rd</sup> Eye Scene plugin;

1. Define the messages for the user entities (generally shapes)
2. Create a .Net project file which references:

- 3esCore library
- 3esRuntime library
- UnityEngine library*
    1. Implement the client messages in C# and equivalent server messages in C++
    2. Create a handler class deriving `Tes.Runtime.MessageHandler`
    3. Implement the abstract methods to:
- Define a Unity `GameObject` to act as a root for all objects created by this handler.
- In `MessageHandler.Initialise()` add the handler's root `GameObject` either to:
  - the scene `root` if the handler's object transformation coordinates are in the server's coordinate frame, or
  - to the `serverRoot` if the handler's object transformation coordinates are in Unity's coordinate frame
- Handle the new messages, creating Unity objects as required
- Discriminate transient and persistent objects based on the object ID
- Serialise shapes on request
    1. Build the plugin DLL
    2. Copy the DLL to the viewer's plugin directory (create this directory alongside the executable).
- Note: plugins are also searched for in 3rd-Eye-Scene_Data/Managed
- See [referencing Unity engine](#referencing-unity-engine) for guidelines on referencing `UnityEngine.dll`

### Referencing Unity engine

The `3esRuntime` library serves an example of how to reference the UnityEngine dll in a cross platform way. This relies on a few assumptions:

- Using conditional blocks in the MSBuild project file (`3esRuntime.csproj`)
- Using the `UNITY_DLL_PATH` environment variable to locate the DLL when possible
- Falling back to finding `UnityEngine.dll` in a known, platform specific location

A discussion of MSBuild files is beyond the scope of this documentation, other than to note that both Visual Studio and Xamarin Studio use MSBuild files to control compilation and both support conditional blocks.

Essentially, `3esRuntime.csproj` references UnityEngine.DLL via the XML code below. The same code can be used to reference the DLL in other .Net project files. Be aware that this code must be manually edited into the project file as Visual Studio and Xamarin Studio do not support creating conditional statements via the UI, but do respect them on reading such project files.

```xml
  <ItemGroup>
    <Reference Include="UnityEngine" Condition="!Exists('$(UNITY_DLL_PATH)') And
Exists('\\Applications\\Unity\\Unity.app\\Contents\\Managed\\UnityEngine.dll') ">
      <HintPath>\Applications\Unity\Unity.app\Contents\Managed\UnityEngine.dll</HintPath>
      <Private>False</Private>
    </Reference>
    <Reference Include="UnityEngine" Condition="!Exists('$(UNITY_DLL_PATH)') And Exists('C:\\Program
Files\\Unity\\Editor\\Data\\Managed\\UnityEngine.dll') "> <HintPath>C:\Program
Files\Unity\Editor\Data\Managed\UnityEngine.dll</HintPath> <Private>False</Private>
    </Reference>
    <Reference Include="UnityEngine" Condition=" Exists('$(UNITY_DLL_PATH)') ">
      <HintPath>$(UNITY_DLL_PATH)\UnityEngine.dll</HintPath>
      <Private>False</Private>
    </Reference>
  </ItemGroup>
```

## Testing a 3rd Eye Scene plugin

The build guidelines above illustrate how to integrate a user plugin into a pre-built version of the 3<sup>rd</sup> Eye Scene viewer. To facilitate debugging and testing, it is recommended that user plugin DLLs are added to the 3<sup>rd</sup> Eye Scene viewer Unity project instead. Build the DLL as described above, then copy the DLL into the viewer Assets folder under: `Assets/plugins`. Unity will automatically include this DLL while running under in the Unity editor and on building the project.

### 3<sup>rd</sup> Eye Scene viewer builds

There is an alternative way to author custom `Tes.Runtime.MessageHandler` implementations. In this option, the custom handlers are packaged as part of the 3<sup>rd</sup> Eye Scene viewer as a customised viewer build. This may be more convenient when distribution is tightly controlled and provides an simpler workflow.

1. Define the messages for the user entities (generally shapes)
2. Open the 3<sup>rd</sup> Eye Scene viewer Unity project.
3. Define client messages and associated `Tes.Runtime.MessageHandler` objects within this project.
4. Implement the client messages in C# and equivalent server messages in C++
5. Build the viewer with user extensions built-in.

It is recommended that the viewer executable name is changed in these circumstances, to reflect the custom build to avoid confusion with the default 3<sup>rd</sup> Eye Scene viewer application.

## Example Plugin

An example plugin project and program are provided as part of the C# core libraries. To see the plugin in in effect:

1. Build the 3esCore C# project.
2. Build the 3<sup>rd</sup> Eye Scene viewer application.
3. Find 3esExamplePlugin.dll under 3esExamplePlugin/bin/Release
4. Copy 3esExamplePlugin.dll into the viewer plugins directory.
    - You will have to create this directory alongside the executable.
        1. Run 3esPluginTest.exe
        2. Run the viewer and connect to the loopback address on the default port.

You should see a white pyramid alongside the axis arrows.
