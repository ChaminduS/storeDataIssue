package pipeline.fifo

import Chisel.log2Ceil
import chisel3._
import pipeline.ports._
import chisel3.util._

/**
  * FIFO IO with enqueue and dequeue ports using the ready/valid interface.
  */
class FifoIO[T <: Data](private val gen: T) extends Bundle {
  val enq = Flipped(new DecoupledIO(gen))
  val deq = new DecoupledIO(gen)
}

abstract class Fifo[T <: Data ]( gen: T, val depth: Int) extends Module  with RequireSyncReset{
  val io = IO(new FifoIO(gen))
  assert(depth > 0, "Number of buffer elements needs to be larger than 0")
}

class robFifo[T <: Data ]( gen: T, depth: Int) extends Fifo(gen:
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
  //printf(p"$io\n")
}

class robResultsFifo[T <: Data ]( gen: T, depth: Int) extends robFifo(gen: T, depth: Int){
  class robWriteport extends Bundle{
    val valid = Input(Bool())
    val data = Input(gen)
    val addr = Input(UInt(log2Ceil(depth).W))
  }

  // Result write ports
  val w1 = IO(new robWriteport)
  val w2 = IO(new robWriteport)
  val w3 = IO(new robWriteport)

  when (w1.valid){
    memReg(w1.addr) := w1.data
  }

  when(w2.valid) {
    memReg(w2.addr) := w2.data
  }

  when(w3.valid) {
    memReg(w3.addr) := w3.data
  }

  val allocatedAddr = IO(Output(UInt(log2Ceil(depth).W)))

  allocatedAddr := writePtr

}

object robVerilog extends App {
  (new chisel3.stage.ChiselStage).emitVerilog(new robFifo(UInt(64.W),64))
}