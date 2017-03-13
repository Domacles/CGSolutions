List<PVector> vec;

Delaunay d;
Scene scene;

public void setup() {
   
  size(700, 700, P3D);

  d = new Delaunay();
  scene = (Scene)new Scene1();
  scene.setEnable();
  background(0);
  
  vec = new ArrayList<PVector>();
  for (int i = 0; i < 100; i++) {
    float r = 90 + random(0.01f);
    float phi = radians(random(-90, 90));
    float theta = radians(random(360));
    vec.add(new PVector(r*cos(phi)*cos(theta), r*sin(phi), r*cos(phi)*sin(theta)));
  }
  d.SetData(vec);
}

float t = 0;
public void draw() {
  t += 0.01f;
  scene = scene.update();
}