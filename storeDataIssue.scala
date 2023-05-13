package storeDataIssue

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

/*------------------------------------------------*/

//Internal signals of the module
val entryValid    = RegInit(true.B)
val fifo_entry    := Cat(fromDecode.rs2Ready, fromDecode.rs2Addr, fromDecode.branchMask, entryValid)

/*------------------------------------------------*/

//Initiating the fifo
val storeQueue = Module(new(RegFifo(UInt(fifo_width.W)), fifo_depth))

//connect the fifo
storeQueue.io.enq.bits  := fifo_entry
storeQueue.io.enq.valid := fromDecode.valid
storeQueue.io.deq.valid := storeQueue.memReg(readPtr)(0).asBool
storeQueue.io.deq.ready := toPRF.ready

//asserting the rs2ready bit of the fifo entries based on the input from Exec unit 
for (i <- 0 to (fifo_depth-1).U ){
  if (storeQueue.memReg(i)(1,prfAddrWidth) === fromExec.prfAddr){
    if (fromExec.readyNow){
      storeQueue.memReg(i)(0) := 1.U
    }
  }
}

//Sending the oldest entry as the output if it is ready
toPRF.rs2Ready    := storeQueue.io.deq.bits(0)
toPRF.rs2Addr     := storeQueue.io.deq.bits(1,prfAddrWidth)
toPRF.branchMask  := storeQueue.io.deq.bits(prfAddrWidth,fifo_width-1) 


}