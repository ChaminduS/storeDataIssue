package storeDataIssue

import constants._

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.util._
import chisel3.experimental.IO


//Initiating the Fifo
class FifoIO extends Bundle {
  val enq = Flipped(new DecoupledIO(UInt(fifo_width.W)))
  val deq = new DecoupledIO(UInt(fifo_width.W))
}

abstract class Fifo( val depth: Int) extends Module  with RequireSyncReset{
  val io = IO(new FifoIO)
  assert(depth > 0, "Number of buffer elements needs to be larger than 0")
}

class sdiFifo( depth: Int) extends Fifo(depth: Int) {
  // branch invalidation port
  val branch = IO(new fromBranchUnit)
  val branchFails = Wire(Vec(depth,Bool()))

  val readReg = RegInit(0.U(log2Ceil(depth).W))
  val nextRead = Mux(readReg === (depth - 1).U, 0.U, readReg + 1.U)
  val writeReg = RegInit(0.U(log2Ceil(depth).W))
  val nextWrite = Mux(writeReg === (depth - 1).U, 0.U, writeReg + 1.U)

  // the register based memory
  val memReg = Mem(depth, UInt(fifo_width.W))
  val readPtr = readReg
  val writePtr = writeReg
  val emptyReg = RegInit(true.B)
  val fullReg = RegInit(false.B)



  val incrRead = WireDefault(false.B)
  val incrWrite = WireDefault(false.B)


  val modifyVal = PriorityEncoder(branchFails)
  val encoderInvalid = !(branchFails(depth-1) === 1.B) & (modifyVal === (depth-1).U)
  val modify = branch.valid & !branch.passOrFail & !encoderInvalid


  when (incrRead) {
    readReg := nextRead
  }


  when (modify){
    //val nextval = Mux(modifyVal === (depth - 1).U, 0.U, modifyVal + 1.U)
    val nextval = modifyVal
    writeReg := nextval
    //fullReg := nextval === readPtr
    emptyReg := nextval === readPtr
  }.elsewhen(incrWrite){
    writeReg := nextWrite
  }


  when(io.deq.ready && io.deq.valid && io.enq.valid && io.enq.ready) {
    memReg(writePtr) := io.enq.bits
    incrWrite := true.B
    incrRead := true.B
  }.elsewhen(io.enq.valid && io.enq.ready) {
    memReg(writePtr) := io.enq.bits
    emptyReg := false.B
    fullReg := nextWrite === readPtr
    incrWrite := true.B
  }.elsewhen(io.deq.ready && io.deq.valid) {
    fullReg := false.B
    emptyReg := nextRead === writePtr
    incrRead := true.B
  }

  io.deq.bits := memReg(readPtr)
  io.enq.ready := (!fullReg | (io.deq.valid & io.deq.ready)) & !modify
  io.deq.valid := !emptyReg & !modify

  //val memVals = Seq.fill(depth)(gen)

  for (i <- 0 until depth) {
    branchFails(i) := !((memReg(i)(9, 6) & branch.branchMask) === 0.U)
  }

  when(branch.valid){
    when(branch.passOrFail){
      for(i<-0 until depth){
        memReg(i) := memReg(i)(9,6) ^ branch.branchMask
      }
    }
  }

}

class composableInterface extends Bundle {
  val ready = Output(Bool())
  val valid = Input(Bool())
}

class fromROBUnit extends Bundle{
  val readyNow  = Input(Bool())
  // val prfAddr   = Input(UInt(prfAddrWidth.W))
}

class fromBranchUnit extends Bundle{
  val passOrFail  = Input(Bool())
  val branchMask  = Input(UInt(branchMaskWidth.W))
  val valid       = Input(Bool())
}

class toPRFUnit extends Bundle{
  val instruction   = Output(UInt(32.W))
  val valid         = Output(Bool())
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
val fromROB    = IO(new fromROBUnit)
val fromBranch  = IO(new fromBranchUnit)
val fromDecode  = IO(new fromDecodeUnit)
val toPRF       = IO(new toPRFUnit)

//Intermediate registers
val toStore_reg = Reg(UInt(fifo_width.W))
val branchMask_reg  = Reg(UInt(branchMaskWidth.W))

//Initiating the fifo
val sdiFifo     = Module(new sdiFifo(fifo_depth))

//Connecting the fifo
sdiFifo.io.enq.valid      := fromDecode.valid
fromDecode.ready          := sdiFifo.io.enq.ready
sdiFifo.io.enq.bits       := Cat(fromDecode.rs2Addr,fromDecode.branchMask)
sdiFifo.branch <> fromBranch

sdiFifo.io.deq.ready      := fromROB.readyNow
toStore_reg               := sdiFifo.io.deq.bits

//Connecting the register to the outputs to PRF
toPRF.rs2Addr     := toStore_reg(5,0)
toPRF.branchMask  := toStore_reg(9,6) 
toPRF.instruction := fromDecode.instruction
toPRF.valid       := sdiFifo.io.deq.valid


}

object sdiVerilog extends App {
  (new chisel3.stage.ChiselStage).emitVerilog(new storeDataIssue())
}