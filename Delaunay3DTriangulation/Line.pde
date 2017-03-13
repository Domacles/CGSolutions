class Line {
    public PVector start, end;
    public Line(PVector start, PVector end) {
        this.start = start;
        this.end = end;
    }

    // 始点と終点をひっくり返す
    public void reverse() {
        PVector tmp = this.start;
        this.start = this.end;
        this.end = tmp;
    }

    // 同じかどうか
    public boolean equals(Line l) {
        if ((this.start == l.start && this.end == l.end)
                || (this.start == l.end && this.end == l.start))
            return true;
        return false;
    }
}