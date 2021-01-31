Scene: Example
Environment:
  AssetPath: Textures/HDR/pink_sunrise_4k.hdr
  Light:
    Direction: [-0.787, -0.73299998, 1]
    Radiance: [1, 1, 1]
    Multiplier: 0.514999986
Entities:
  - Entity: 15861629587505754
    TagComponent:
      Tag: Sprite_01
    TransformComponent:
      Position: [-1, -1, 0]
      Rotation: [0, 0, 0, 0]
      Scale: [1, 1, 1]
    SpriteRendererComponent:
      Color: [1, 0, 0, 1]
      TilingFactor: 1.0
    MeshComponent:
      AssetPath: Models/Hazel/Cube1m.fbx
  - Entity: 15223077898852293773
    TagComponent:
      Tag: Sprite_02
    TransformComponent:
      Position: [1, -1, 0]
      Rotation: [0, 0, 0, 0]
      Scale: [1, 1, 1]
    SpriteRendererComponent:
      Color: [0, 1, 0, 1]
      TilingFactor: 1.0
    MeshComponent:
      AssetPath: Models/Hazel/Cube1m.fbx
  - Entity: 2157107598622182863
    TagComponent:
      Tag: Sprite_03
    TransformComponent:
      Position: [1, 1, 0]
      Rotation: [0, 0, 0, 0]
      Scale: [1, 1, 1]
    SpriteRendererComponent:
      Color: [1, 1, 0, 1]
      TilingFactor: 1.0
    MeshComponent:
      AssetPath: Models/Hazel/Cube1m.fbx
  - Entity: 8080964283681139153
    TagComponent:
      Tag: Sprite_04
    TransformComponent:
      Position: [-1, 1, 0]
      Rotation: [0, 0, 0, 0]
      Scale: [1, 1, 1]
    SpriteRendererComponent:
      Color: [0, 0, 1, 1]
      TilingFactor: 1.0
    MeshComponent:
      AssetPath: Models/Hazel/Cube1m.fbx
  - Entity: 1352995477042327524
    TagComponent:
      Tag: Camera
    TransformComponent:
      Position: [0, 0, 10]
      Rotation: [0, 0, 0, 0]
      Scale: [1, 1, 1]
    CameraComponent:
      Camera:
        ProjectionType: 0
        PerspectiveFOV: 0.785398185
        PerspectiveNear: 0.100000001
        PerspectiveFar: 1000
        OrthographicSize: 10
        OrthographicNear: -1000
        OrthographicFar: 1000
      Primary: true
      FixedAspectRatio: false
