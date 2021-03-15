package mp.paschalis.llvm.demo.TestHelpers;

/**
 * Created by paschalis on 04/08/2017.
 */

public class Human {

  int height_ = 12;
  int kilos_ = 11;

  public Human() {
  }

  public int getHeight() {
      return height_;
  }
    public int getKilos() {
        return kilos_;
    }
  public Human(int height, int kilos) {
    this.height_ += height;
    this.kilos_ += kilos;
  }

  public int getRes() {
    return height_ + kilos_;
  }

}
