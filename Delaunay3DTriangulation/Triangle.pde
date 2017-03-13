/**
 * 三角形クラス
 *
 * @author tercel
 */
class Triangle {
    public PVector v1, v2, v3;
    public Triangle(PVector v1, PVector v2, PVector v3) {
        this.v1 = v1;
        this.v2 = v2;
        this.v3 = v3;
    }

    // 法線を求める
    // 頂点は左回りの順であるとする
    public PVector getNormal() {
        PVector edge1 = new PVector(v2.x-v1.x, v2.y-v1.y, v2.z-v1.z);
        PVector edge2 = new PVector(v3.x-v1.x, v3.y-v1.y, v3.z-v1.z);

        // クロス積
        PVector normal = edge1.cross(edge2);
        normal.normalize();
        return normal;
    }

    // 面を裏返す（頂点の順序を逆に）
    public void turnBack() {
        PVector tmp = this.v3;
        this.v3 = this.v1;
        this.v1 = tmp;
    }

    // 線分のリストを得る
    public Line[] getLines() {
        Line[] l = {
            new Line(v1, v2),
            new Line(v2, v3),
            new Line(v3, v1)
        };
        return l;
    }

    // 同じかどうか。すげー簡易的なチェック
    public boolean equals(Triangle t) {
        Line[] lines1 = this.getLines();
        Line[] lines2 = t.getLines();

        int cnt = 0;
        for(int i = 0; i < lines1.length; i++) {
            for(int j = 0; j < lines2.length; j++) {
                if (lines1[i].equals(lines2[j]))
                    cnt++;
            }
        }
        if (cnt == 3) return true;
        else return false;

    }
}