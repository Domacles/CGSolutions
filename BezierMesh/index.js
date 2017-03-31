let B = function (x, t) {
    if (x < 0.0 || x > 3.0 || t < 0.0 || t > 1.0)
        return 0;
    let res = Math.pow(t, x) * Math.pow((1 - t), 3 - x);
    if (x === 0 || x === 3)
        return res;
    if (x === 1 || x === 2)
        return 3 * res;
    return 0;
};

let Bezier = function (vertices, x, s) {
    if (x > s || s === 0)
        return createVector(0, 0, 0);
    let t = (x * 1.0) / (s * 1.0);
    let res = createVector(0, 0, 0);
    for (let i = 0; i < 4; i++)
        res.add(p5.Vector.mult(vertices[i], B(i, t)));
    return res;
};

let Test_Vertices = function () {
    const len = 300;
    let control_vertices = Array();
    for (let i = 0; i < 4; i++) {
        let vs = new Array();
        for (let j = 0; j < 4; j++) {
            let point = createVector((i - 1) * len, (j - 1) * len, Math.random() * len / 10);
            if ((i == 1 || i == 2) && (j == 1 || j == 2))
                point.add(createVector(0, 0, len));
            vs.push(point);
        }
        control_vertices.push(vs);
    }
    return control_vertices;
};

let Random_Vertices = function () {
    let control_vertices = Array();
    for (let i = 0; i < 4; i++) {
        let vs = new Array();
        for (let j = 0; j < 4; j++) {
            let point = p5.Vector.mult(p5.Vector.random3D(), 800);
            vs.push(point);
        }
        control_vertices.push(vs);
    }
    return control_vertices;
}

let Mesh = function (vertices, faces) {
    let mesh = new Object();
    mesh.vertices = vertices;
    mesh.faces = faces;
    return mesh;
};

let BezierMesh = function (control_vertices) {
    const size = 50;

    let faces = new Array();
    let vertices = new Array();
    let tmp_vertices = new Array();
    let center = createVector(0, 0, 0);

    for (let i = 0; i < 4; i++) {
        let B_vertices = new Array();
        for (let j = 0; j < size; j++) {
            let vs = control_vertices[i];
            let B_point = Bezier(vs, j, size);
            B_vertices.push(B_point);
        }
        tmp_vertices.push(B_vertices);
    }

    for (let i = 0; i < size; i++) {
        for (let j = 0; j < size; j++) {
            let vs = [tmp_vertices[0][i], tmp_vertices[1][i], tmp_vertices[2][i], tmp_vertices[3][i]];
            let B_point = Bezier(vs, j, size);
            vertices.push(B_point);
        }
    }

    for (let i = 1; i < size; i++)
        for (let j = 1; j < size; j++) {
            faces.push({
                v0: (i - 1) * size + (j - 1),
                v1: (i - 1) * size + (j - 0),
                v2: (i - 0) * size + (j - 0),
                v3: (i - 0) * size + (j - 1)
            });
        }

    return Mesh(vertices, faces);
};

let control_vertices;
let global_Mesh;

function setup() {
    createCanvas(windowWidth - 20, windowHeight - 20, WEBGL);
    control_vertices = Test_Vertices();
    global_Mesh = BezierMesh(control_vertices);
    console.log("V num : " + global_Mesh.vertices.length);
    console.log("F num : " + global_Mesh.faces.length);
}

function draw() {
    rotateX(frameCount * 0.005);
    rotateY(frameCount * 0.005);
    rotateZ(frameCount * 0.005);
    for (let i = 0; i < global_Mesh.faces.length; i++) {
        let face = global_Mesh.faces[i];
        let v0 = global_Mesh.vertices[face.v0], v1 = global_Mesh.vertices[face.v1];
        let v2 = global_Mesh.vertices[face.v2], v3 = global_Mesh.vertices[face.v3];
        noFill();
        beginShape();
        vertex(v0.x, v0.y, v0.z);
        vertex(v1.x, v1.y, v1.z);
        vertex(v2.x, v2.y, v2.z);
        vertex(v3.x, v3.y, v3.z);
        endShape(CLOSE);
    }
}