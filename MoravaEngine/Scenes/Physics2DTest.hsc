Scene: Scene Name
Environment:
  AssetPath: Textures/HDR/pink_sunrise_4k.hdr
  Light:
    Direction: [-0.787, -0.73299998, 1]
    Radiance: [1, 1, 1]
    Multiplier: 0.514999986
Entities:
  - Entity: 15861629587505754
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [-18.2095661, 39.2518234, 0]
      Rotation: [0.967056513, 0, 0, -0.254561812]
      Scale: [4.47999525, 4.47999525, 4.48000002]
    MeshComponent:
      AssetPath: Models/Hazel/Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [2.24000001, 2.24000001]
  - Entity: 15223077898852293773
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [5.37119865, 43.8762894, 0]
      Rotation: [0.977883637, 0, 0, -0.209149718]
      Scale: [4.47999668, 4.47999668, 4.48000002]
    MeshComponent:
      AssetPath: Models/Hazel/Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [2.24000001, 2.24000001]
  - Entity: 2157107598622182863
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [-7.60411549, 44.1442184, 0]
      Rotation: [0.989285827, 0, 0, 0.145991713]
      Scale: [4.47999287, 4.47999287, 4.48000002]
    MeshComponent:
      AssetPath: Models/Hazel/Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 0.5
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [2.24000001, 2.24000001]
  - Entity: 8080964283681139153
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [-0.739211679, 37.7653275, 0]
      Rotation: [0.956475914, 0, 0, -0.291811317]
      Scale: [5, 2, 2]
    MeshComponent:
      AssetPath: Models/Hazel/Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 0.25
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [2.5, 1]
  - Entity: 1352995477042327524
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [-8.32969856, 30.4078159, 0]
      Rotation: [0.781595349, 0, 0, 0.623785794]
      Scale: [14.000001, 4.47999334, 4.48000002]
    MeshComponent:
      AssetPath: Models/Hazel/Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 3
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [7, 2.24000001]
  - Entity: 935615878363259513
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [6.88031197, 31.942337, 0]
      Rotation: [0.986578286, 0, 0, 0.163288936]
      Scale: [4.47999954, 4.47999954, 4.48000002]
    MeshComponent:
      AssetPath: Models/Hazel/Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [2.24000001, 2.24000001]
  - Entity: 14057422478420564497
    TagComponent:
      Tag: Player
    TransformComponent:
      Position: [0, 22.774044, 0]
      Rotation: [0.942591429, 0, 0, -0.333948225]
      Scale: [6.00000048, 6.00000048, 4.48000002]
    ScriptComponent:
      ModuleName: Example.PlayerCube
      StoredFields:
        - Name: HorizontalForce
          Type: 1
          Data: 10
        - Name: VerticalForce
          Type: 1
          Data: 10
    MeshComponent:
      AssetPath: Models/Hazel/Sphere1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 1
    CircleCollider2DComponent:
      Offset: [0, 0]
      Radius: 3
  - Entity: 1289165777996378215
    TagComponent:
      Tag: Cube
    TransformComponent:
      Position: [0, 0, 0]
      Rotation: [1, 0, 0, 0]
      Scale: [50, 1, 50]
    ScriptComponent:
      ModuleName: Example.Sink
      StoredFields:
        - Name: SinkSpeed
          Type: 1
          Data: 0
    MeshComponent:
      AssetPath: Models/Hazel/Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 0
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [25, 0.5]
  - Entity: 5178862374589434728
    TagComponent:
      Tag: Camera
    TransformComponent:
      Position: [0, 25, 79.75]
      Rotation: [0.995602965, -0.0936739072, 0, 0]
      Scale: [1, 0.999999821, 0.999999821]
    ScriptComponent:
      ModuleName: Example.BasicController
      StoredFields:
        - Name: Speed
          Type: 1
          Data: 12
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
  - Entity: 3948844418381294888
    TagComponent:
      Tag: Box
    TransformComponent:
      Position: [-1.48028564, 49.5945244, -2.38418579e-07]
      Rotation: [0.977883637, 0, 0, -0.209149733]
      Scale: [1.99999976, 1.99999976, 2]
    MeshComponent:
      AssetPath: Models/Hazel/Cube1m.fbx
    RigidBody2DComponent:
      BodyType: 1
      Mass: 1
    BoxCollider2DComponent:
      Offset: [0, 0]
      Size: [1, 1]
