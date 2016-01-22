package Common

import Chisel._
import Node._
import Constants._


class ioDebug extends Bundle
{
  val error_mode  = Bool(OUTPUT);
}

class ioHost(w: Int) extends Bundle
{
  val in = new FIFOIO()(Bits(width = w)).flip
  val out = new FIFOIO()(Bits(width = w))
}

class PCRReq extends Bundle
{
  val rw = Bool()
  val addr = Bits(width = 6)
  val data = Bits(width = 64)
}

class ioHTIF extends Bundle
{
  val reset = Bool(INPUT)
  val debug = new ioDebug
  val pcr_req = (new FIFOIO) { new PCRReq }.flip
  val pcr_rep = (new FIFOIO) { Bits(width = 64) }
  val ipi_req = (new FIFOIO) { Bits(width = log2Up(NTILES)) }
  val ipi_rep = (new FIFOIO) { Bool() }.flip
}

class rocketHTIF(w: Int, ncores: Int, co: CoherencePolicyWithUncached) extends Component
{
  val io = new Bundle {
    val host = new ioHost(w)
    val cpu = Vec(ncores) { new ioHTIF().flip }
    val mem = new ioTileLink
  }

  val short_request_bits = 64
  val long_request_bits = 576
  require(short_request_bits % w == 0)

  val rx_count_w = 13 + log2Up(64) - log2Up(w) // data size field is 12 bits
  val rx_count = Reg(resetVal = UFix(0,rx_count_w))
  val rx_shifter = Reg() { Bits(width = short_request_bits) }
  val rx_shifter_in = Cat(io.host.in.bits, rx_shifter(short_request_bits-1,w))
  val next_cmd = rx_shifter_in(3,0)
  val cmd = Reg() { Bits() }
  val size = Reg() { Bits() }
  val pos = Reg() { Bits() }
  val seqno = Reg() { Bits() }
  val addr = Reg() { Bits() }
  when (io.host.in.valid && io.host.in.ready) {
    rx_shifter := rx_shifter_in
    rx_count := rx_count + UFix(1)
    when (rx_count === UFix(short_request_bits/w-1)) {
      cmd := next_cmd
      size := rx_shifter_in(15,4)
      pos := rx_shifter_in(15,4+OFFSET_BITS-3)
      seqno := rx_shifter_in(23,16)
      addr := rx_shifter_in(63,24)
    }
  }

  val rx_word_count = (rx_count >> UFix(log2Up(short_request_bits/w)))
  val rx_word_done = io.host.in.valid && rx_count(log2Up(short_request_bits/w)-1,0).andR
  val packet_ram_depth = long_request_bits/short_request_bits-1
  val packet_ram = Vec(packet_ram_depth) { Reg() { Bits(width = short_request_bits) } }
  when (rx_word_done && io.host.in.ready) {
    packet_ram(rx_word_count(log2Up(packet_ram_depth)-1,0) - UFix(1)) := rx_shifter_in
  }

  val cmd_readmem :: cmd_writemem :: cmd_readcr :: cmd_writecr :: cmd_ack :: cmd_nack :: Nil = Enum(6) { UFix() }

  val pcr_addr = addr(io.cpu(0).pcr_req.bits.addr.width-1, 0)
  val pcr_coreid = if (ncores == 1) UFix(0) else addr(20+log2Up(ncores),20)
  val pcr_wdata = packet_ram(0)

  val bad_mem_packet = size(OFFSET_BITS-1-3,0).orR || addr(OFFSET_BITS-1-3,0).orR
  val nack = Mux(cmd === cmd_readmem || cmd === cmd_writemem, bad_mem_packet,
             Mux(cmd === cmd_readcr || cmd === cmd_writecr, size != UFix(1),
             Bool(true)))

  val tx_count = Reg(resetVal = UFix(0, rx_count_w))
  val tx_subword_count = tx_count(log2Up(short_request_bits/w)-1,0)
  val tx_word_count = tx_count(rx_count_w-1, log2Up(short_request_bits/w))
  val packet_ram_raddr = tx_word_count(log2Up(packet_ram_depth)-1,0) - UFix(1)
  when (io.host.out.valid && io.host.out.ready) {
    tx_count := tx_count + UFix(1)
  }

  val rx_done = rx_word_done && Mux(rx_word_count === UFix(0), next_cmd != cmd_writemem && next_cmd != cmd_writecr, rx_word_count === size || rx_word_count(log2Up(packet_ram_depth)-1,0) === UFix(0))
  val tx_size = Mux(!nack && (cmd === cmd_readmem || cmd === cmd_readcr || cmd === cmd_writecr), size, UFix(0))
  val tx_done = io.host.out.ready && tx_subword_count.andR && (tx_word_count === tx_size || tx_word_count > UFix(0) && packet_ram_raddr.andR)

  val mem_acked = Reg(resetVal = Bool(false))
  val mem_gxid = Reg() { Bits() }
  val mem_needs_ack = Reg() { Bool() }
  val mem_nacked = Reg(resetVal = Bool(false))
  when (io.mem.xact_rep.valid) { 
    mem_acked := Bool(true)
    mem_gxid := io.mem.xact_rep.bits.global_xact_id
    mem_needs_ack := io.mem.xact_rep.bits.require_ack  
  }
  io.mem.xact_rep.ready := Bool(true)
  when (io.mem.xact_abort.valid) { mem_nacked := Bool(true) }
  io.mem.xact_abort.ready := Bool(true)

  val state_rx :: state_pcr_req :: state_pcr_resp :: state_mem_req :: state_mem_wdata :: state_mem_wresp :: state_mem_rdata :: state_mem_finish :: state_tx :: Nil = Enum(9) { UFix() }
  val state = Reg(resetVal = state_rx)

  when (state === state_rx && rx_done) {
    val rx_cmd = Mux(rx_word_count === UFix(0), next_cmd, cmd)
    state := Mux(rx_cmd === cmd_readmem || rx_cmd === cmd_writemem, state_mem_req,
             Mux(rx_cmd === cmd_readcr || rx_cmd === cmd_writecr, state_pcr_req,
             state_tx))
  }

  val mem_cnt = Reg(resetVal = UFix(0, log2Up(REFILL_CYCLES)))
  val x_init = new Queue(1)(new TransactionInit)
  when (state === state_mem_req && x_init.io.enq.ready) {
    state := Mux(cmd === cmd_writemem, state_mem_wdata, state_mem_rdata)
  }
  when (state === state_mem_wdata && io.mem.xact_init_data.ready) {
    when (mem_cnt.andR)  {
      state := state_mem_wresp
    }
    mem_cnt := mem_cnt + UFix(1)
  }
  when (state === state_mem_wresp) {
    when (mem_nacked) {
      state := state_mem_req
      mem_nacked := Bool(false)
    }
    when (mem_acked) {
      state := state_mem_finish
      mem_acked := Bool(false)
    }
  }
  when (state === state_mem_rdata) {
    when (mem_nacked) {
      state := state_mem_req
      mem_nacked := Bool(false)
    }
    when (io.mem.xact_rep.valid) {
      when (mem_cnt.andR)  {
        state := state_mem_finish
      }
      mem_cnt := mem_cnt + UFix(1)
    }
    mem_acked := Bool(false)
  }
  when (state === state_mem_finish && io.mem.xact_finish.ready) {
    state := Mux(cmd === cmd_readmem || pos === UFix(1),  state_tx, state_rx)
    pos := pos - UFix(1)
    addr := addr + UFix(1 << OFFSET_BITS-3)
  }
  when (state === state_tx && tx_done) {
    when (tx_word_count === tx_size) {
      rx_count := UFix(0)
      tx_count := UFix(0)
    }
    state := Mux(cmd === cmd_readmem && pos != UFix(0), state_mem_req, state_rx)
  }

  var mem_req_data: Bits = null
  for (i <- 0 until MEM_DATA_BITS/short_request_bits) {
    val idx = Cat(mem_cnt, UFix(i, log2Up(MEM_DATA_BITS/short_request_bits)))
    when (state === state_mem_rdata && io.mem.xact_rep.valid) {
      packet_ram(idx) := io.mem.xact_rep.bits.data((i+1)*short_request_bits-1, i*short_request_bits)
    }
    mem_req_data = Cat(packet_ram(idx), mem_req_data)
  }
  x_init.io.enq.valid := state === state_mem_req
  x_init.io.enq.bits.x_type := Mux(cmd === cmd_writemem, co.getTransactionInitTypeOnUncachedWrite, co.getTransactionInitTypeOnUncachedRead)
  x_init.io.enq.bits.address := addr.toUFix >> UFix(OFFSET_BITS-3)
  io.mem.xact_init <> x_init.io.deq
  io.mem.xact_init_data.valid:= state === state_mem_wdata
  io.mem.xact_init_data.bits.data := mem_req_data
  io.mem.xact_finish.valid := (state === state_mem_finish) && mem_needs_ack
  io.mem.xact_finish.bits.global_xact_id := mem_gxid
  io.mem.probe_req.ready := Bool(false)
  io.mem.probe_rep.valid := Bool(false)
  io.mem.probe_rep_data.valid := Bool(false)
  io.mem.incoherent := Bool(true)

  val pcr_mux = (new Mux1H(ncores)) { Bits(width = 64) }
  for (i <- 0 until ncores) {
    val my_reset = Reg(resetVal = Bool(true))
    val my_ipi = Reg(resetVal = Bool(false))
    val rdata = Reg() { Bits() }

    val cpu = io.cpu(i)
    val me = pcr_coreid === UFix(i)
    cpu.pcr_req.valid := state === state_pcr_req && me && pcr_addr != PCR_RESET
    cpu.pcr_req.bits.rw := cmd === cmd_writecr
    cpu.pcr_req.bits.addr := pcr_addr
    cpu.pcr_req.bits.data := pcr_wdata
    cpu.reset := my_reset

    when (cpu.ipi_rep.ready) {
      my_ipi := Bool(false)
    }
    cpu.ipi_rep.valid := my_ipi
    cpu.ipi_req.ready := Bool(true)
    for (j <- 0 until ncores) {
      when (io.cpu(j).ipi_req.valid && io.cpu(j).ipi_req.bits === UFix(i)) {
        my_ipi := Bool(true)
      }
    }

    when (cpu.pcr_req.valid && cpu.pcr_req.ready) {
      state := state_pcr_resp
    }
    when (state === state_pcr_req && me && pcr_addr === PCR_RESET) {
      when (cmd === cmd_writecr) {
        my_reset := pcr_wdata(0)
      }
      rdata := my_reset.toBits
      state := state_tx
    }

    cpu.pcr_rep.ready := Bool(true)
    when (cpu.pcr_rep.valid) {
      rdata := cpu.pcr_rep.bits
      state := state_tx
    }

    pcr_mux.io.sel(i) := me
    pcr_mux.io.in(i) := rdata
  }

  val tx_cmd = Mux(nack, cmd_nack, cmd_ack)
  val tx_cmd_ext = Cat(Bits(0, 4-tx_cmd.getWidth), tx_cmd)
  val tx_header = Cat(addr, seqno, tx_size, tx_cmd_ext)
  val tx_data = Mux(tx_word_count === UFix(0), tx_header,
                Mux(cmd === cmd_readcr || cmd === cmd_writecr, pcr_mux.io.out,
                packet_ram(packet_ram_raddr)))

  io.host.in.ready := state === state_rx
  io.host.out.valid := state === state_tx
  io.host.out.bits := tx_data >> Cat(tx_count(log2Up(short_request_bits/w)-1,0), Bits(0, log2Up(w)))
}
