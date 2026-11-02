Scene: Pink Cube
Environment:
  AssetPath: Textures/HDR/pink_sunrise_4k.hdr
  Light:
    Direction: [-0.787, -0.73299998, 1]
    Radiance: [1, 1, 1]
    Multiplier: 0.514999986
Entities:
  - Entity: 2157107598622182863
    TagComponent:
      Tag: Camera
    TransformComponent:
      Position: [0, 1.25, 3]
      Rotation: [-0.436332315, 0, 0, 0]
      Scale: [1, 1, 1]
    CameraComponent:
      Camera:
        ProjectionType: 0
        PerspectiveFOV: 0.785398185
        PerspectiveNear: 0.00999999978
        PerspectiveFar: 1000
        OrthographicSize: 10
        OrthographicNear: -1000
        OrthographicFar: 1000
      Primary: true
      FixedAspectRatio: false
  - Entity: 8080964283681139153
    TagComponent:
      Tag: Top
    TransformComponent:
      Position: [0, 0.5, 0]
      Rotation: [1.57079637, 0.785398185, 0, 0]
      Scale: [1, 1, 1]
    MeshComponent:
      AssetPath: Models/cube.obj
    SpriteRendererComponent:
      Color: [0.999726295, 0, 1, 1]
      TilingFactor: 1
  - Entity: 1352995477042327524
    TagComponent:
      Tag: Left
    TransformComponent:
      Position: [-0.349999994, 0, 0.349999994]
      Rotation: [0, -0.785398185, 0, 0]
      Scale: [1, 1, 1]
    MeshComponent:
      AssetPath: Models/cube.obj
    SpriteRendererComponent:
      Color: [1, 0.312236309, 0.954968333, 1]
      TilingFactor: 1
  - Entity: 935615878363259513
    TagComponent:
      Tag: Right
    TransformComponent:
      Position: [0.349999994, 0, 0.349999994]
      Rotation: [0, 0.785398185, 0, 0]
      Scale: [1, 1, 1]
    MeshComponent:
      AssetPath: Models/cube.obj
    SpriteRendererComponent:
      Color: [0.856380403, 0, 0.856540084, 1]
      TilingFactor: 1
