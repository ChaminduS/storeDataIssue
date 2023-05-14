package storeDataIssue

import constants._

import chisel3._
import chisel3.experimental.BundleLiterals._
import chisel3.util._

class composableInterface extends Bundle {
  val ready = Output(Bool())
  val valid = Input(Bool())
}

class fromROBUnit extends Bundle{
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

//Initiating the Fifo
class FifoIO[T <: Data](private val gen: T) extends Bundle {
  val enq = Flipped(new DecoupledIO(gen))
  val deq = new DecoupledIO(gen)
}

abstract class Fifo[T <: Data ]( gen: T, val depth: Int) extends Module  with RequireSyncReset{
  val io = IO(new FifoIO(gen))
  assert(depth > 0, "Number of buffer elements needs to be larger than 0")
}

class sdiFifo[T <: Data ]( gen: T, depth: Int) extends Fifo(gen:
  T, depth: Int) {

  val incrRead = WireDefault(false.B)
  val incrWrite = WireDefault(false.B)

  val modify = IO(Input(Bool()))
  val modifyVal = IO(Input(UInt(log2Ceil(depth).W)))

  val readReg = RegInit (0.U(log2Ceil(depth).W))
  val nextRead = Mux(readReg === (depth -1).U, 0.U, readReg + 1.U)
  when (incrRead) {
    readReg := nextRead
  }

  val writeReg = RegInit(0.U(log2Ceil(depth).W))
  val nextWrite = Mux(writeReg === (depth - 1).U, 0.U, writeReg + 1.U)


  when (modify){
    //val nextval = Mux(modifyVal === (depth - 1).U, 0.U, modifyVal + 1.U)
    val nextval = modifyVal
    writeReg := nextval
    //fullReg := nextval === readPtr
    emptyReg := nextval === readPtr
  }.elsewhen(incrWrite){
    writeReg := nextWrite
  }

  // the register based memory
  val memReg = Mem(depth , gen)
  val readPtr = readReg
  val writePtr = writeReg
  val emptyReg = RegInit(true.B)
  val fullReg = RegInit(false.B)


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
}

class storeDataIssue extends Module{

//Inputs and Outputs of the Module
  val fromRob    = IO(new fromROBUnit)
  val fromBranch  = IO(new fromBranchUnit)
  val fromDecode  = IO(new fromDecodeUnit)
  val toPRF       = IO(new toPRFUnit)

//Intermediate registers
val toStore_reg = Reg(UInt(fifo_width.W))
val branchMask_reg  = Reg(UInt(branchMaskWidth.W))

//Initiating the fifo
val sdiFifo     = Module(new sdiFifo(UInt(fifo_width.W), fifo_depth))

//Connecting the fifo
sdiFifo.io.enq.valid      := fromDecode.valid
fromDecode.ready          := sdiFifo.io.enq.ready
toPRF.valid               := sdiFifo.io.deq.valid
sdiFifo.io.deq.ready      := fromRob.readyNow
toStore_reg               := sdiFifo.io.deq.bits

//Connecting the register to the outputs to PRF
toPRF.rs2Addr     := toStore_reg(0,5)
toPRF.branchMask  := toStore_reg(6,9) 

//Defining a priority encoder



//branch Mask logic



}

object sdiVerilog extends App {
  (new chisel3.stage.ChiselStage).emitVerilog(new storeDataIssue())
}