package storeDataIssue

import constants._

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.util._

class composableInterface extends Bundle {
  val ready = Output(Bool())
  val valid = Input(Bool())
}

class fromExecUnit extends Bundle{
  val readyNow  = Input(Bool())
  val prfAddr   = Input(UInt(prfAddrWidth.W))
}

class fromBranchUnit extends Bundle{
  val passOrFail  = Input(Bool())
  val branchMask  = Input(UInt(branchMaskWidth.W))
  val valid       = Input(Bool())
}

class toPRFUnit extends composableInterface{
  val instruction   = Output(UInt(32.W))
  val rs2Ready      = Output(Bool())
  val rs2Addr       = Output(UInt(prfAddrWidth.W))
  val branchMask    = Output(UInt(branchMaskWidth.W))
}

class fromDecodeUnit extends composableInterface{
  val instruction   = Input(UInt(32.W))
  val rs2Addr       = Input(UInt(prfAddrWidth.W))
  val rs2Ready      = Input(Bool())
  val branchMask    = Input(UInt(branchMaskWidth.W))
}


class storeDataIssue extends Module{

//Inputs and Outputs of the Module
  val fromExec    = IO(new fromExecUnit)
  val fromBranch  = IO(new fromBranchUnit)
  val fromDecode  = IO(new fromDecodeUnit)
  val toPRF       = IO(new toPRFUnit)

//Internal signals of the module


//Initiating the Fifo
val storeQueue = Reg(Vec(fifo_depth, UInt(fifo_width.W)))

//
  def counter(depth: Int , incr: Bool): (UInt , UInt) = {
    val cntReg = RegInit (0.U(log2Ceil(depth).W))
    val nextVal = Mux(cntReg === (depth -1).U, 0.U, cntReg + 1.U)
    when (incr) {
      cntReg := nextVal
    }
    (cntReg , nextVal)
  }

  val (readPtr , nextRead)    = counter(fifo_depth , incrRead)
  val (writePtr , nextWrite ) = counter(fifo_depth , incrWrite)

}