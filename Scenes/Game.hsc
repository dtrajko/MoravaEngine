Scene: Game
Environment:
  AssetPath: Textures/HDR/pink_sunrise_4k.hdr
  Light:
    Direction: [-0.787, -0.73299998, 1]
    Radiance: [1, 1, 1]
    Multiplier: 0.514999986
Entities:
  - Entity: 15861629587505754
    TagComponent:
      Tag: Map Generator
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [1, 0, 0, 0]
      Scale: [1, 1, 1]
  - Entity: 15223077898852293773
    TagComponent:
      Tag: Directional Light
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [1, 0, 0, 0]
      Scale: [1, 1, 1]
    DirectionalLightComponent:
      Radiance: [1, 1, 1]
      CastShadows: true
      SoftShadows: true
      LightSize: 0.5
  - Entity: 2157107598622182863
    TagComponent:
      Tag: Platform
    TransformComponent:
      Position: [0, 0, -16]
      Rotation: [1, 0, 0, 0]
      Scale: [3, 0.5, 20]
    MeshComponent:
      AssetPath: Models/cube.obj
  - Entity: 8080964283681139153
    TagComponent:
      Tag: Player
    TransformComponent:
      Position: [0, 4, 0]
      Rotation: [1, 0, 0, 0]
      Scale: [1, 1, 1]
    MeshComponent:
      AssetPath: Models/sphere.obj
  - Entity: 1352995477042327524
    TagComponent:
      Tag: Directional Light
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [0.87758255, 0, -0.47942555, 0]
      Scale: [1, 1, 1]
    DirectionalLightComponent:
      Radiance: [1, 1, 1]
      CastShadows: true
      SoftShadows: true
      LightSize: 0.5
  - Entity: 935615878363259513
    TagComponent:
      Tag: Map Generator
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [1, 0, 0, 0]
      Scale: [1, 1, 1]
