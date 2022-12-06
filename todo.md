# TODO

- Modernise the API
  - Update to RAII practices

## Protocol breaking changes

- Remove `SIdPointCloud` and associated classes. `SIdMeshShape` and `SIdMeshSet` suffice.
  - Update `MeshResource` to have a `drawSize()` member targetting point size, line width or voxel size depending on draw type.

## Client

- Modernise the API
  - Add a template API which emulates the current macro API
    - Must compile to nothing without `TES_ENABLE`
    - Aiming to completely replace the macro API

## Viewer

- Change shape maps using `Id` object as a key to just `uint32_t`. The category is not part of the key.

Advanced:

- Interpolate transforms over render frames for smoother animation.
