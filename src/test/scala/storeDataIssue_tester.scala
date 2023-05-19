import chisel3._
import chisel3.util._
import chisel3.Driver
import chisel3.iotesters._

class storeDataIssue_tester (dut : storeDataIssue) extends PeekPokeTester(dut){
    poke (dut.fromDecode.valid, true.B)
    poke (dut.fromDecode.rs2Addr, "b001001".U)
    poke (dut.fromDecode.branchMask, "b1001".U)
    step(1)
    poke (dut.fromDecode.valid, true.B)
    poke (dut.fromDecode.rs2Addr, "b001011".U)
    poke (dut.fromDecode.branchMask, "b1101".U)
    step(1)
    poke (dut.fromBranch.valid, true.B)
    poke (dut.fromBranch.passOrFail, true.B)
    poke (dut.fromBranch.branchMask, "b1001".U)
    step(1)
    poke (dut.fromRob.readyNow, true.B)
    step(1)
    expect (dut.toPRF.valid, true.B)
    expect (dut.toPRF.rs2Addr, "b001001".U)
    expect (dut.toPRF.branchMask, "")



}


object storeDataIssue_tester extends App{
    chisel3.iotesters.Driver(()=>new storeDataIssue) {c =>
        new storeDataIssue(c)
    }
}