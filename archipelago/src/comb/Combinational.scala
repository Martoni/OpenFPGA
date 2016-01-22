package Combinational

import Chisel._
import scala.collection.mutable.HashMap
import scala.util.Random

class Combinational extends Component {
  val io = new Bundle {
    val x   = UFix(INPUT,  16)
    val y   = UFix(INPUT,  16)
    val z   = UFix(OUTPUT, 16)
  }
  io.z := io.x + io.y
}

class CombinationalTests(c: Combinational) extends Tester(c, Array(c.io)) {
  defTests {
    var allGood = true
    val vars    = new HashMap[Node, Node]()
    val rnd     = new Random()
    val maxInt  = 1 << 16
    for (i <- 0 until 10) {
      vars.clear()
      val x = rnd.nextInt(maxInt)
      val y = rnd.nextInt(maxInt)
      vars(c.io.x) = UFix(x)
      vars(c.io.y) = UFix(y)
      vars(c.io.z) = UFix((x + y)&(maxInt-1))
      allGood = step(vars) && allGood
    }
    allGood
  }
}

object CombinationalMain {
  def main(args: Array[String]): Unit = { 
    val chiselArgs = args.tail

    chiselMainTest(chiselArgs, () => new Combinational) {
      dut => new CombinationalTests(dut)
    }
  }
}
