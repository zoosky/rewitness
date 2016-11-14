/*
 * vim: set ft=rust:
 * vim: set ft=reason:
 */
let module Make (Gl: Gl.t) => {
  /* Setting up the Gl utils functions */
  type glCamera = {projectionMatrix: Gl.Mat4.t, modelViewMatrix: Gl.Mat4.t};
  type glEnv = {camera: glCamera, window: Gl.Window.t, gl: Gl.contextT};

  /** Will mutate the projectionMatrix to be an ortho matrix with the given boundaries.
   *  See this link for quick explanation of what this is.
   *  https://shearer12345.github.io/graphics/assets/projectionPerspectiveVSOrthographic.png
   */
  let setProjection (window: Gl.Window.t) (camera: glCamera) =>
    Gl.Mat4.ortho
      out::camera.projectionMatrix
      left::0.
      right::(float_of_int (Gl.Window.getWidth window))
      bottom::0.
      top::(float_of_int (Gl.Window.getHeight window))
      near::0.
      far::100.;
  let resetCamera (camera: glCamera) => Gl.Mat4.identity camera.modelViewMatrix;
  let buildGlEnv window::(window: Gl.Window.t) :glEnv => {
    let gl = Gl.Window.getContext window;
    let glCamera = {projectionMatrix: Gl.Mat4.create (), modelViewMatrix: Gl.Mat4.create ()};
    let env = {camera: glCamera, window, gl};
    let canvasWidth = Gl.Window.getWidth window;
    let canvasHeight = Gl.Window.getHeight window;
    Gl.viewport gl 0 0 canvasWidth canvasHeight;
    Gl.clearColor gl 0.0 0.0 0.0 1.0;
    Gl.clear gl (Constants.color_buffer_bit lor Constants.depth_buffer_bit);
    env
  };
  let getProgram
      env::(env: glEnv)
      vertexShader::vertexShaderSource
      fragmentShader::fragmentShaderSource
      :option Gl.programT => {
    let vertex_shader = Gl.createShader env.gl Constants.vertex_shader;
    Gl.shaderSource env.gl vertex_shader vertexShaderSource;
    Gl.compileShader env.gl vertex_shader;
    let compiledCorrectly =
      Gl.getShaderParameter context::env.gl shader::vertex_shader paramName::Gl.Compile_status == 1;
    if compiledCorrectly {
      let fragment_shader = Gl.createShader env.gl Constants.fragment_shader;
      Gl.shaderSource env.gl fragment_shader fragmentShaderSource;
      Gl.compileShader env.gl fragment_shader;
      let compiledCorrectly =
        Gl.getShaderParameter context::env.gl shader::fragment_shader paramName::Gl.Compile_status == 1;
      if compiledCorrectly {
        let program = Gl.createProgram env.gl;
        Gl.attachShader context::env.gl program::program shader::vertex_shader;
        /* Gl.deleteShader */
        Gl.attachShader context::env.gl program::program shader::fragment_shader;
        /* Gl.deleteShader */
        Gl.linkProgram env.gl program;
        let linkedCorrectly =
          Gl.getProgramParameter context::env.gl program::program paramName::Gl.Link_status == 1;
        if linkedCorrectly {
          Some program
        } else {
          print_endline @@
          "Linking error: " ^ Gl.getProgramInfoLog context::env.gl program::program;
          None
        }
      } else {
        print_endline @@
        "Fragment shader error: " ^ Gl.getShaderInfoLog context::env.gl shader::fragment_shader;
        None
      }
    } else {
      print_endline @@
      "Vertex shader error: " ^ Gl.getShaderInfoLog context::env.gl shader::vertex_shader;
      None
    }
  };

  /** Setting up the game's datatypes. **/
  type tileType = {bottom: bool, left: bool, top: bool, right: bool};
  type tileSideType =
    | Bottom
    | Left
    | Top
    | Right
    | Center;
  type floatPointType = {x: float, y: float};
  type intPointType = {x: int, y: int};
  type wCoordType =
    | WCoord floatPointType;
  type gCoordType =
    | GCoord intPointType;
  type pCoordType =
    | PCoord intPointType;
  type tileSidePointType = {position: pCoordType, tileSide: tileSideType};
  type puzzleType = {
    startTile: pCoordType,
    endTile: tileSidePointType,
    grid: list (list tileType)
  };
  type tilePointType = {tile: tileType, position: pCoordType};
  type gameStateType = {
    mutable currentPath: list tilePointType,
    mutable lineEdge: option gCoordType
  };
  let module Color = {
    let red = (1., 0., 0.);
    let yellow = (0.97, 0.7, 0.);
    let brightYellow = (0.985, 0.88, 0.3);
    let brown = (0.28, 0.21, 0.02);
    let grey = (0.27, 0.3, 0.32);
  };
  let module B = {
    let b = {bottom: true, left: false, top: false, right: false};
    let l = {bottom: false, left: true, top: false, right: false};
    let t = {bottom: false, left: false, top: true, right: false};
    let r = {bottom: false, left: false, top: false, right: true};
    let bl = {bottom: true, left: true, top: false, right: false};
    let br = {bottom: true, left: false, top: false, right: true};
    let lt = {bottom: false, left: true, top: true, right: false};
    let tr = {bottom: false, left: false, top: true, right: true};
    let bt = {bottom: true, left: false, top: true, right: false};
    let lr = {bottom: false, left: true, top: false, right: true};
    let blt = {bottom: true, left: true, top: true, right: false};
    let ltr = {bottom: false, left: true, top: true, right: true};
    let btr = {bottom: true, left: false, top: true, right: true};
    let blr = {bottom: true, left: true, top: false, right: true};
    let bltr = {bottom: true, left: true, top: true, right: true};
    let n = {bottom: false, left: false, top: false, right: false};
  };
  let lineWeight = 30;
  let lineWeightf = float_of_int lineWeight;
  let windowSize = 800;
  let frameWidth = lineWeight;
  let windowSizef = float_of_int windowSize;
  let examplePuzzle = {
    startTile: PCoord {x: 4, y: 2},
    endTile: {position: PCoord {x: 2, y: 6}, tileSide: Top},
    grid: [
      [B.br, B.lr, B.blt, B.br, B.bl, B.br, B.bl],
      [B.bt, B.b, B.tr, B.lt, B.tr, B.lt, B.bt],
      [B.bt, B.btr, B.bl, B.r, B.blr, B.lr, B.blt],
      [B.t, B.bt, B.btr, B.lr, B.lt, B.b, B.bt],
      [B.r, B.blt, B.tr, B.blr, B.blr, B.blt, B.t],
      [B.b, B.t, B.b, B.tr, B.lt, B.tr, B.bl],
      [B.tr, B.lr, B.ltr, B.lr, B.lr, B.lr, B.lt]
    ]
  };
  let preprocess target shader =>
    if (target == "web") {
      "#version 100 \n precision highp float; \n" ^ shader
    } else if (
      target == "native"
    ) {
      "#version 120 \n" ^ shader
    } else {
      shader
    };
  let vertexShaderSource =
    preprocess
      Gl.target
      {|
       attribute vec3 aVertexPosition;
       attribute vec4 aVertexColor;

       uniform mat4 uMVMatrix;
       uniform mat4 uPMatrix;

       varying vec4 vColor;

       void main(void) {
         gl_Position = uPMatrix * uMVMatrix * vec4(aVertexPosition, 1.0);
         vColor = aVertexColor;
       }|};
  let fragmentShaderSource =
    preprocess
      Gl.target
      {|
       varying vec4 vColor;

       void main(void) {
         gl_FragColor = vColor;
       }
     |};
  let start () => {
    let window = Gl.Window.init argv::Sys.argv;
    Gl.Window.setWindowSize window::window width::windowSize height::windowSize;
    let env = buildGlEnv window::window;
    let program =
      switch (
        getProgram env::env vertexShader::vertexShaderSource fragmentShader::fragmentShaderSource
      ) {
      | None => failwith "Could not create the program and/or the shaders. Aborting."
      | Some program => program
      };
    Gl.useProgram env.gl program;
    let positionAttrib = Gl.getAttribLocation env.gl program "aVertexPosition";
    Gl.enableVertexAttribArray env.gl positionAttrib;
    let colorAttrib = Gl.getAttribLocation env.gl program "aVertexColor";
    Gl.enableVertexAttribArray env.gl colorAttrib;
    let pMatrixUniform = Gl.getUniformLocation env.gl program "uPMatrix";
    Gl.uniformMatrix4fv
      context::env.gl location::pMatrixUniform value::env.camera.projectionMatrix;
    let mvMatrixUniform = Gl.getUniformLocation env.gl program "uMVMatrix";
    Gl.uniformMatrix4fv
      context::env.gl location::mvMatrixUniform value::env.camera.modelViewMatrix;
    setProjection env.window env.camera;

    /** We define the drawRect and drawCircle here so we can capture in their closure the following variables
     *  env, positionAttrib, pMatrixUniform, mvMatrixUniform.
     *  I'm not sure what the easiest way is to keep the attributes defined in the string shader and the
     *  "pointers" to those that we get with `getAttribLocation`.
     *
     */
    let drawRect
        vertexBuffer::vertexBuffer
        colorBuffer::colorBuffer
        width::width
        height::height
        color::(r, g, b)
        position::(GCoord {x, y}) => {
      resetCamera env.camera;

      /** Setup vertices to be sent to the GPU **/
      let square_vertices = [|
        float_of_int @@ x + width,
        float_of_int @@ y + height,
        0.0,
        float_of_int x,
        float_of_int @@ y + height,
        0.0,
        float_of_int @@ x + width,
        float_of_int y,
        0.0,
        float_of_int x,
        float_of_int y,
        0.0
      |];
      Gl.bindBuffer context::env.gl target::Constants.array_buffer buffer::vertexBuffer;
      Gl.bufferData
        context::env.gl
        target::Constants.array_buffer
        data::(Gl.Float32 square_vertices)
        usage::Constants.static_draw;
      Gl.vertexAttribPointer env.gl positionAttrib 3 Constants.float_ false 0 0;

      /** Setup colors to be sent to the GPU **/
      let square_colors = ref [];
      for i in 0 to 3 {
        square_colors := [r, g, b, 1., ...!square_colors]
      };
      Gl.bindBuffer context::env.gl target::Constants.array_buffer buffer::colorBuffer;
      Gl.bufferData
        context::env.gl
        target::Constants.array_buffer
        data::(Gl.Float32 (Array.of_list !square_colors))
        usage::Constants.static_draw;
      Gl.vertexAttribPointer env.gl colorAttrib 4 Constants.float_ false 0 0;
      Gl.uniformMatrix4fv env.gl pMatrixUniform env.camera.projectionMatrix;
      Gl.uniformMatrix4fv env.gl mvMatrixUniform env.camera.modelViewMatrix;
      Gl.drawArrays env.gl Constants.triangle_strip 0 4
    };
    let drawCircle
        vertexBuffer::vertexBuffer
        colorBuffer::colorBuffer
        radius::radius
        color::(r, g, b)
        position::(GCoord {x, y}) => {
      resetCamera env.camera;

      /** Instantiate a list of points for the circle and bind to the circleBuffer. **/
      let circle_vertex = ref [];
      for i in 0 to 360 {
        let deg2grad = 3.14159 /. 180.;
        let degInGrad = float_of_int i *. deg2grad;
        let floatRadius = float_of_int radius;
        circle_vertex := [
          cos degInGrad *. floatRadius +. float_of_int x,
          sin degInGrad *. floatRadius +. float_of_int y,
          0.,
          ...!circle_vertex
        ]
      };
      Gl.bindBuffer env.gl Constants.array_buffer vertexBuffer;
      Gl.bufferData
        context::env.gl
        target::Constants.array_buffer
        data::(Gl.Float32 (Array.of_list !circle_vertex))
        usage::Constants.static_draw;
      Gl.vertexAttribPointer env.gl positionAttrib 3 Constants.float_ false 0 0;

      /** Instantiate color array **/
      let circle_colors = ref [];
      for i in 0 to 360 {
        circle_colors := [r, g, b, 1., ...!circle_colors]
      };
      Gl.bindBuffer env.gl Constants.array_buffer colorBuffer;
      Gl.bufferData
        context::env.gl
        target::Constants.array_buffer
        data::(Gl.Float32 (Array.of_list !circle_colors))
        usage::Constants.static_draw;
      Gl.vertexAttribPointer env.gl colorAttrib 4 Constants.float_ false 0 0;
      Gl.uniformMatrix4fv env.gl pMatrixUniform env.camera.projectionMatrix;
      Gl.uniformMatrix4fv env.gl mvMatrixUniform env.camera.modelViewMatrix;
      Gl.drawArrays env.gl Constants.triangle_fan 0 360
    };
    let myDrawRect =
      drawRect vertexBuffer::(Gl.createBuffer env.gl) colorBuffer::(Gl.createBuffer env.gl);
    let myDrawCircle =
      drawCircle vertexBuffer::(Gl.createBuffer env.gl) colorBuffer::(Gl.createBuffer env.gl);
    let centerPoint puzzleSize::puzzleSize position::(GCoord {x, y}) =>
      GCoord {
        x: x + windowSize / 2 - puzzleSize * 3 * lineWeight / 2,
        y: y + windowSize / 2 - puzzleSize * 3 * lineWeight / 2
      };
    let toGameCoord (PCoord {x, y}) => GCoord {x: x * 3 * lineWeight, y: y * 3 * lineWeight};
    let getTileCenter (GCoord {x, y}) =>
      GCoord {x: x + 3 * lineWeight / 2, y: y + 3 * lineWeight / 2};
    let drawCell tile::{bottom, left, top, right} color::color position::(GCoord {x, y}) => {
      let numOfSides = ref 0;
      if bottom {
        myDrawRect
          width::lineWeight
          height::(2 * lineWeight - lineWeight / 2)
          color::color
          position::(GCoord {x: x + lineWeight, y});
        numOfSides := !numOfSides + 1
      };
      if left {
        myDrawRect
          width::(2 * lineWeight - lineWeight / 2)
          height::lineWeight
          color::color
          position::(GCoord {x, y: y + lineWeight});
        numOfSides := !numOfSides + 1
      };
      if top {
        myDrawRect
          width::lineWeight
          height::(2 * lineWeight - lineWeight / 2)
          color::color
          position::(GCoord {x: x + lineWeight, y: y + lineWeight + lineWeight / 2});
        numOfSides := !numOfSides + 1
      };
      if right {
        myDrawRect
          width::(2 * lineWeight - lineWeight / 2)
          height::lineWeight
          color::color
          position::(GCoord {x: x + lineWeight + lineWeight / 2, y: y + lineWeight});
        numOfSides := !numOfSides + 1
      };
      if (!numOfSides >= 2) {
        myDrawCircle
          radius::(lineWeight / 2) color::color position::(getTileCenter (GCoord {x, y}))
      } else {
        let GCoord tileCenter = getTileCenter (GCoord {x, y});
        myDrawRect
          width::lineWeight
          height::lineWeight
          color::color
          position::(GCoord {x: tileCenter.x - lineWeight / 2, y: tileCenter.y - lineWeight / 2})
      }
    };
    let drawPuzzle puzzle::puzzle => {
      let puzzleSize = List.length puzzle.grid;
      List.iteri
        (
          fun y row =>
            List.iteri
              (
                fun x tile =>
                  drawCell
                    tile::tile
                    color::Color.brown
                    position::(
                      centerPoint puzzleSize::puzzleSize position::(toGameCoord (PCoord {x, y}))
                    )
              )
              row
        )
        (List.rev puzzle.grid);
      myDrawCircle
        radius::lineWeight
        color::Color.brown
        position::(
          getTileCenter (
            centerPoint puzzleSize::puzzleSize position::(toGameCoord puzzle.startTile)
          )
        )
    };
    let getTileSide puzzle::puzzle tile::tile position::(GCoord {x: px, y: py}) => {
      let puzzleSize = List.length puzzle.grid;
      let GCoord {x: tx, y: ty} = getTileCenter (
        centerPoint puzzleSize::puzzleSize position::(toGameCoord tile.position)
      );
      if (py < ty - 7) {
        Bottom
      } else if (px < tx - 7) {
        Left
      } else if (py > ty + 7) {
        Top
      } else if (
        px > tx + 7
      ) {
        Right
      } else {
        Center
      }
    };

    /** Draw the tip of the line if any. Just the tip. **/
    let drawTip puzzle::puzzle prevTile::maybePrevTile curTile::curTile lineEdge::(GCoord lineEdge) => {
      let puzzleSize = List.length puzzle.grid;
      let lineEdgeTileSide = getTileSide puzzle::puzzle tile::curTile position::(GCoord lineEdge);
      let GCoord tileCenter = getTileCenter (
        centerPoint puzzleSize::puzzleSize position::(toGameCoord curTile.position)
      );
      let drawSomething () =>
        switch lineEdgeTileSide {
        | Bottom =>
          myDrawRect
            width::lineWeight
            height::(tileCenter.y - lineEdge.y)
            color::Color.brightYellow
            position::(GCoord {x: tileCenter.x - lineWeight / 2, y: lineEdge.y})
        | Left =>
          myDrawRect
            width::(tileCenter.x - lineEdge.x)
            height::lineWeight
            color::Color.brightYellow
            position::(GCoord {x: lineEdge.x, y: tileCenter.y - lineWeight / 2})
        | Top =>
          myDrawRect
            width::lineWeight
            height::(lineEdge.y - tileCenter.y)
            color::Color.brightYellow
            position::(GCoord {x: tileCenter.x - lineWeight / 2, y: tileCenter.y})
        | Right =>
          myDrawRect
            width::(lineEdge.x - tileCenter.x)
            height::lineWeight
            color::Color.brightYellow
            position::(GCoord {x: tileCenter.x, y: tileCenter.y - lineWeight / 2})
        | Center => ()
        };
      switch maybePrevTile {
      | None => drawSomething ()
      | Some prevTile =>
        let prevTileSide =
          getTileSide
            puzzle::puzzle
            tile::curTile
            position::(
              getTileCenter (
                centerPoint puzzleSize::puzzleSize position::(toGameCoord prevTile.position)
              )
            );
        let distanceFromEdge = 3 * lineWeight / 2;
        switch prevTileSide {
        | Top =>
          myDrawRect
            width::lineWeight
            height::(tileCenter.y + distanceFromEdge - lineEdge.y)
            color::Color.brightYellow
            position::(GCoord {x: tileCenter.x - lineWeight / 2, y: lineEdge.y})
        | Right =>
          myDrawRect
            width::(tileCenter.x + distanceFromEdge - lineEdge.x)
            height::lineWeight
            color::Color.brightYellow
            position::(GCoord {x: lineEdge.x, y: tileCenter.y - lineWeight / 2})
        | Bottom =>
          myDrawRect
            width::lineWeight
            height::(lineEdge.y - (tileCenter.y - distanceFromEdge))
            color::Color.brightYellow
            position::(
              GCoord {x: tileCenter.x - lineWeight / 2, y: tileCenter.y - distanceFromEdge}
            )
        | Left =>
          myDrawRect
            width::(lineEdge.x - (tileCenter.x - distanceFromEdge))
            height::lineWeight
            color::Color.brightYellow
            position::(
              GCoord {x: tileCenter.x - distanceFromEdge, y: tileCenter.y - lineWeight / 2}
            )
        | Center => assert false
        };
        /* This means we're moving away from the center */
        if (prevTileSide != lineEdgeTileSide) {
          drawSomething ();
          myDrawCircle
            radius::(lineWeight / 2) color::Color.brightYellow position::(GCoord tileCenter)
        }
      }
    };
    let maybeToTilePoint puzzle::puzzle position::(GCoord {x, y}) => {
      let puzzleSize = float_of_int (List.length puzzle.grid);
      let uncenteredPoint: floatPointType = {
        x: float_of_int x +. puzzleSize *. 3. *. lineWeightf /. 2. -. windowSizef /. 2.,
        y: float_of_int y +. puzzleSize *. 3. *. lineWeightf /. 2. -. windowSizef /. 2.
      };
      let puzzleCoord: floatPointType = {
        x: uncenteredPoint.x /. (3. *. lineWeightf),
        y: uncenteredPoint.y /. (3. *. lineWeightf)
      };
      if (
        puzzleCoord.x < 0. ||
        puzzleCoord.x >= puzzleSize || puzzleCoord.y < 0. || puzzleCoord.y >= puzzleSize
      ) {
        None
      } else {
        Some {
          tile:
            List.nth
              (List.nth (List.rev puzzle.grid) (int_of_float puzzleCoord.y))
              (int_of_float puzzleCoord.x),
          position: PCoord {x: int_of_float puzzleCoord.x, y: int_of_float puzzleCoord.y}
        }
      }
    };
    let getDistance (GCoord {x: x1, y: y1}) (GCoord {x: x2, y: y2}) => {
      let dx = x2 - x1;
      let dy = y2 - y1;
      sqrt @@ float_of_int (dx * dx + dy * dy)
    };
    let printTile tile =>
      print_endline @@
      "bottom: " ^
      string_of_bool tile.bottom ^
      ", left: " ^
      string_of_bool tile.left ^
      ", top: " ^ string_of_bool tile.top ^ ", right: " ^ string_of_bool tile.right;
    let addSide curSide::ret cur::(PCoord cur) other::(PCoord other) =>
      if (other.y < cur.y) {
        {...ret, bottom: true}
      } else if (other.x < cur.x) {
        {...ret, left: true}
      } else if (
        other.y > cur.y
      ) {
        {...ret, top: true}
      } else if (
        other.x > cur.x
      ) {
        {...ret, right: true}
      } else {
        assert false
      };
    let min3 a b c => min a (min b c);
    let max3 a b c => max a (max b c);

    /** Warning: cool iterative algorithm below.
     *  TODO(schmavery): I... don't remember how this works, could you fill in some explanation here.
     */
    let getOptimalLineEdge
        puzzle::puzzle
        gameState::gameState
        possibleDirs::possibleDirs
        lineEdge::(GCoord {x: lex, y: ley})
        mousePos::(GCoord {x: mx, y: my}) => {
      let mousePos = GCoord {x: mx, y: my};
      let lineEdge = GCoord {x: lex, y: ley};
      let lineEdgeTile =
        switch (maybeToTilePoint puzzle::puzzle position::lineEdge) {
        | None => assert false
        | Some x => x
        };
      let puzzleSize = List.length puzzle.grid;
      let GCoord {x: centerX, y: centerY} = getTileCenter (
        centerPoint puzzleSize::puzzleSize position::(toGameCoord lineEdgeTile.position)
      );
      let minDist = ref (getDistance mousePos lineEdge);
      let minCoord = ref lineEdge;
      if possibleDirs.bottom {
        let potentialLineEdge =
          GCoord {x: centerX, y: min (max3 my centerY (centerY - 3 * lineWeight / 2)) (ley - 1)};
        let potentialDistance = getDistance potentialLineEdge mousePos;
        if (potentialDistance < !minDist) {
          minDist := potentialDistance;
          minCoord := potentialLineEdge
        }
      };
      if possibleDirs.left {
        let potentialLineEdge =
          GCoord {x: min (max3 mx centerX (centerX - 3 * lineWeight / 2)) (lex - 1), y: centerY};
        let potentialDistance = getDistance potentialLineEdge mousePos;
        if (potentialDistance < !minDist) {
          minDist := potentialDistance;
          minCoord := potentialLineEdge
        }
      };
      if possibleDirs.top {
        let potentialLineEdge =
          GCoord {x: centerX, y: max (min3 my centerY (centerY + 3 * lineWeight / 2)) (ley + 1)};
        let potentialDistance = getDistance potentialLineEdge mousePos;
        if (potentialDistance < !minDist) {
          minDist := potentialDistance;
          minCoord := potentialLineEdge
        }
      };
      if possibleDirs.right {
        let potentialLineEdge =
          GCoord {x: max (min3 mx centerX (centerX + 3 * lineWeight / 2)) (lex + 1), y: centerY};
        let potentialDistance = getDistance potentialLineEdge mousePos;
        if (potentialDistance < !minDist) {
          minDist := potentialDistance;
          minCoord := potentialLineEdge
        }
      };
      let maybeNextTile = maybeToTilePoint puzzle::puzzle position::!minCoord;
      switch maybeNextTile {
      | None => gameState
      | Some nextTile =>
        let getNextPath currentPath tile nextTile =>
          switch currentPath {
          | [] => [
              {
                tile: addSide curSide::B.n cur::tile.position other::nextTile.position,
                position: tile.position
              }
            ]
          | [head, ...tail] when head.position == nextTile.position => tail
          | [head, ...tail] =>
            if (List.length (List.filter (fun x => x.position == nextTile.position) tail) == 0) {
              [
                {
                  tile:
                    addSide
                      curSide::(addSide curSide::B.n cur::tile.position other::head.position)
                      cur::tile.position
                      other::nextTile.position,
                  position: tile.position
                },
                head,
                ...tail
              ]
            } else {
              [head, ...tail]
            }
          };
        let nextLineEdge =
          switch gameState.currentPath {
          | [] => Some !minCoord
          | [head] => Some !minCoord
          | [head, ...tail] =>
            if (List.length (List.filter (fun x => x.position == nextTile.position) tail) == 0) {
              Some !minCoord
            } else {
              Some lineEdge
            }
          };
        if (nextTile == lineEdgeTile) {
          {...gameState, lineEdge: nextLineEdge}
        } else {
          switch (getTileSide puzzle::puzzle tile::nextTile position::!minCoord) {
          | Bottom =>
            if nextTile.tile.bottom {
              {
                currentPath: getNextPath gameState.currentPath lineEdgeTile nextTile,
                lineEdge: nextLineEdge
              }
            } else {
              gameState
            }
          | Left =>
            if nextTile.tile.left {
              {
                currentPath: getNextPath gameState.currentPath lineEdgeTile nextTile,
                lineEdge: nextLineEdge
              }
            } else {
              gameState
            }
          | Top =>
            if nextTile.tile.top {
              {
                currentPath: getNextPath gameState.currentPath lineEdgeTile nextTile,
                lineEdge: nextLineEdge
              }
            } else {
              gameState
            }
          | Right =>
            if nextTile.tile.right {
              {
                currentPath: getNextPath gameState.currentPath lineEdgeTile nextTile,
                lineEdge: nextLineEdge
              }
            } else {
              gameState
            }
          | Center => assert false
          }
        }
      }
    };
    let didClickOnStartTile puzzle::puzzle mousePos::mousePos => {
      let puzzleSize = List.length puzzle.grid;
      let circleCenter = getTileCenter (
        centerPoint puzzleSize::puzzleSize position::(toGameCoord puzzle.startTile)
      );
      let distance = getDistance circleCenter mousePos;
      distance <= lineWeightf
    };
    let onMouseDown puzzle::puzzle gameState::gameState button::button state::state x::x y::y => {
      let mousePos = GCoord {x, y: windowSize - y};
      let puzzleSize = List.length puzzle.grid;
      switch button {
      | Gl.Events.LEFT_BUTTON =>
        if (state == Gl.Events.DOWN) {
          switch gameState.lineEdge {
          | None when didClickOnStartTile puzzle::puzzle mousePos::mousePos =>
            gameState.lineEdge =
              Some (
                getTileCenter (
                  centerPoint puzzleSize::puzzleSize position::(toGameCoord puzzle.startTile)
                )
              )
          | Some lineEdge =>
            let lineEdgeTile =
              switch (maybeToTilePoint puzzle::puzzle position::lineEdge) {
              | None => assert false
              | Some x => x
              };
            let lineTileSide = getTileSide puzzle::puzzle tile::lineEdgeTile position::lineEdge;
            let endTileCenter = getTileCenter (
              centerPoint puzzleSize::puzzleSize position::(toGameCoord puzzle.endTile.position)
            );
            if (
              lineEdgeTile.position == puzzle.endTile.position &&
              lineTileSide == puzzle.endTile.tileSide && (
                getDistance lineEdge endTileCenter >= lineWeightf || lineTileSide == Center
              )
            ) {
              assert false
            } else {
              gameState.lineEdge = None;
              gameState.currentPath = []
            }
          | _ => ()
          }
        }
      | _ => ()
      }
    };

    /** Computes the line drawn when the mouse is moved and mutates the gameState. **/
    let onMouseMove puzzle::puzzle gameState::gameState x::x y::y => {
      let mousePos = GCoord {x, y: windowSize - y};
      let rec loop i =>
        switch gameState.lineEdge {
        | None => ()
        | Some lineEdge =>
          switch (maybeToTilePoint puzzle::puzzle position::lineEdge) {
          | None => assert false
          | Some tilePoint =>
            let nextGameState =
              switch (getTileSide puzzle::puzzle tile::tilePoint position::lineEdge) {
              | Bottom
              | Top =>
                getOptimalLineEdge
                  puzzle::puzzle
                  gameState::gameState
                  possibleDirs::B.bt
                  lineEdge::lineEdge
                  mousePos::mousePos
              | Left
              | Right =>
                getOptimalLineEdge
                  puzzle::puzzle
                  gameState::gameState
                  possibleDirs::B.lr
                  lineEdge::lineEdge
                  mousePos::mousePos
              | Center =>
                getOptimalLineEdge
                  puzzle::puzzle
                  gameState::gameState
                  possibleDirs::tilePoint.tile
                  lineEdge::lineEdge
                  mousePos::mousePos
              };
            if (nextGameState != gameState) {
              gameState.currentPath = nextGameState.currentPath;
              gameState.lineEdge = nextGameState.lineEdge;
              if (i > 0) {
                loop (i - 1)
              }
            }
          }
        };
      loop 10
    };

    /** Main render function.
     *  It'll render the whole maze and the line being drawn.
     */
    let render puzzle::puzzle gameState::gameState time => {
      Gl.clear env.gl (Constants.color_buffer_bit lor Constants.depth_buffer_bit);
      myDrawRect
        width::windowSize height::windowSize color::Color.grey position::(GCoord {x: 0, y: 0});
      myDrawRect
        width::(windowSize - frameWidth * 2)
        height::(windowSize - frameWidth * 2)
        color::Color.yellow
        position::(GCoord {x: frameWidth, y: frameWidth});
      drawPuzzle puzzle::puzzle;
      let puzzleSize = List.length puzzle.grid;
      let GCoord endTileCenter = getTileCenter (
        centerPoint puzzleSize::puzzleSize position::(toGameCoord puzzle.endTile.position)
      );
      switch puzzle.endTile {
      | {tileSide: Bottom} =>
        myDrawCircle
          radius::(lineWeight / 2)
          color::Color.brown
          position::(GCoord {x: endTileCenter.x, y: endTileCenter.y - 3 * lineWeight / 2})
      | {tileSide: Left} =>
        myDrawCircle
          radius::(lineWeight / 2)
          color::Color.brown
          position::(GCoord {x: endTileCenter.x - 3 * lineWeight / 2, y: endTileCenter.y})
      | {tileSide: Top} =>
        myDrawCircle
          radius::(lineWeight / 2)
          color::Color.brown
          position::(GCoord {x: endTileCenter.x, y: endTileCenter.y + 3 * lineWeight / 2})
      | {tileSide: Right} =>
        myDrawCircle
          radius::(lineWeight / 2)
          color::Color.brown
          position::(GCoord {x: endTileCenter.x + 3 * lineWeight / 2, y: endTileCenter.y})
      | {tileSide: Center} => ()
      };
      switch gameState.lineEdge {
      | None => ()
      | Some lineEdge =>
        myDrawCircle
          radius::lineWeight
          color::Color.brightYellow
          position::(
            getTileCenter (
              centerPoint puzzleSize::puzzleSize position::(toGameCoord puzzle.startTile)
            )
          );
        ignore @@
        List.map
          (
            fun tilePoint =>
              drawCell
                tile::tilePoint.tile
                color::Color.brightYellow
                position::(
                  centerPoint puzzleSize::puzzleSize position::(toGameCoord tilePoint.position)
                )
          )
          gameState.currentPath;
        switch (maybeToTilePoint puzzle::puzzle position::lineEdge) {
        | None => assert false
        | Some curTile =>
          switch gameState.currentPath {
          | [] => drawTip puzzle::puzzle prevTile::None curTile::curTile lineEdge::lineEdge
          | [head, ...tail] =>
            drawTip puzzle::puzzle prevTile::(Some head) curTile::curTile lineEdge::lineEdge
          }
        };
        myDrawCircle radius::(lineWeight / 2) color::Color.brightYellow position::lineEdge
      }
    };
    let gameState = {currentPath: [], lineEdge: None};
    let puzzle = examplePuzzle;
    Gl.render
      window::window
      mouseDown::(onMouseDown puzzle::puzzle gameState::gameState)
      mouseMove::(onMouseMove puzzle::puzzle gameState::gameState)
      displayFunc::(render puzzle::puzzle gameState::gameState)
      ()
  };
};
