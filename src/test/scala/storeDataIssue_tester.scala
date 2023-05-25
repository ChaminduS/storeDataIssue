package storeDataIssue

import org.scalatest._

import chisel3._
import chisel3.experimental.BundleLiterals._
import chiseltest._
import org.scalatest.flatspec.AnyFlatSpec
import org.scalatest.matchers.should.Matchers

class storeDataIssue_tester extends AnyFlatSpec with ChiselScalatestTester {
    behavior of "StoreDataIssue"
    
    it should "provide required outputs to PRF" in{
        test (new storeDataIssue) { dut =>
            dut.fromDecode.valid.poke(true.B)
            dut.fromDecode.rs2Addr.poke("b001101".U)
            dut.fromDecode.branchMask.poke("b1001".U)
            dut.clock.step()
            dut.fromDecode.valid.poke(true.B)
            dut.fromDecode.rs2Addr.poke("b001011".U)
            dut.fromDecode.branchMask.poke("b1100".U)
            dut.clock.step()
            dut.fromDecode.valid.poke(true.B)
            dut.fromDecode.rs2Addr.poke("b001111".U)
            dut.fromDecode.branchMask.poke("b1000".U)
            dut.clock.step()
            dut.fromBranch.valid.poke(true.B)
            dut.fromBranch.passOrFail.poke(true.B)
            dut.fromBranch.branchMask.poke("b1001".U)
            dut.clock.step()
            dut.fromROB.readyNow.poke(true.B)
            dut.clock.step()
            dut.toPRF.valid.expect(true.B)
            dut.toPRF.rs2Addr.expect("b001101".U)
            dut.toPRF.branchMask.expect("b0000".U)
            println("branch mask value :" + dut.toPRF.branchMask.peek())
            println("rs2Addr value :" + dut.toPRF.rs2Addr.peek())
        }

    }

}

// object storeDataIssue_tester extends App{
//     chisel3.iotesters.Driver(()=>new storeDataIssue) {c =>
//         new storeDataIssue_tester(c)
//     }
// }