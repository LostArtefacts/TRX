-- Test harness for the Lua surface tests, mirroring the C one in harness.h.
--
-- The C runner builds the world - the fake engine, the real bridges, the real
-- data/scripting modules - and then hands over to a file like items.lua. What a
-- script can see, the tests see, so the assertions are written the way a level
-- builder would write them.

local M = { cases = {}, passed = 0, failed = 0 }

function M.test(name, fn)
  M.cases[#M.cases + 1] = { name = name, fn = fn }
end

-- Asserts that `fn` raises, and that the message mentions `needle`. The message
-- is part of the contract: "is read-only" and "stale ITEM handle" are what a
-- builder reads, so a write that fails for some other reason is not a pass.
function M.raises(fn, needle)
  local ok, err = pcall(fn)
  assert(not ok, "expected an error, but the call succeeded")
  if needle ~= nil then
    assert(
      tostring(err):find(needle, 1, true),
      ("expected an error matching '%s', got: %s"):format(needle, tostring(err))
    )
  end
end

function M.report()
  -- A suite that registered nothing reports "0 failed" and the runner takes the
  -- zero for a pass, which reads exactly like a suite that ran.
  assert(#M.cases > 0, "the suite registered no cases")

  local order = {}
  for i = 1, #M.cases do
    order[i] = i
  end

  -- The cases share one Lua state, so they must not depend on each other. Set
  -- TRX_TEST_SEED to shuffle them: an accidental dependency then fails the suite
  -- instead of lying dormant behind the file order.
  local seed = tonumber(os.getenv("TRX_TEST_SEED") or "")
  if seed ~= nil then
    math.randomseed(seed)
    for i = #order, 2, -1 do
      local j = math.random(i)
      order[i], order[j] = order[j], order[i]
    end
    print(("(shuffled, seed %d)"):format(seed))
  end

  for _, i in ipairs(order) do
    local case = M.cases[i]
    -- Every case starts from the same world. A handle held from an earlier case
    -- is stale by design.
    fake.reset()
    local ok, err = pcall(case.fn)
    if ok then
      M.passed = M.passed + 1
      print("  PASS  " .. case.name)
    else
      M.failed = M.failed + 1
      print("  FAIL  " .. case.name)
      print("        " .. tostring(err))
    end
  end

  print(("\n%d passed, %d failed, %d total"):format(M.passed, M.failed, M.passed + M.failed))
  return M.failed
end

return M
