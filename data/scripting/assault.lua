local raw = trxc.assault

trx.assault = {
  start = raw.start,
  stop = raw.stop,
  reset = raw.reset,
  quad = {
    start = raw.quad.start,
    stop = raw.quad.stop,
    reset = raw.quad.reset,
  },
}
