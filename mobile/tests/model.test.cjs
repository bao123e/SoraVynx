const {test} = require('node:test');
const assert = require('node:assert/strict');
const {parseStatus, canToggle, bounded, PLAYER, EVENT} = require('../src/model.ts');
const ready = {phase:3, hooks:2, capabilities:[0,1,0,1,0,0,0,0,1], enabled:Array(9).fill(0), damage:2, vacuum:3, score:2};
test('Malformed or incomplete ready responses cannot enable controls', () => {
  for (const value of [null, {}, {phase:3}, {...ready,capabilities:[1]}, {...ready,hooks:0}, {...ready,score:101}, {...ready,enabled:Array(9).fill(true)}]) {
    assert.throws(() => parseStatus(JSON.stringify(value)));
  }
  assert.throws(() => parseStatus('not json'));
});
test('Only confirmed capabilities are selectable', () => {
  const status = parseStatus(JSON.stringify(ready));
  assert.equal(canToggle(status,1),true);
  assert.equal(canToggle(status,2),false);
  for (const phase of [-1,0,1,2,4,5,6]) {assert.equal(canToggle({...status,phase},1),false);}
  assert.equal(canToggle(status,0),false);
  assert.equal(canToggle(status,9),false);
});
test('Backend acknowledgement determines displayed state', () => {
  const status = parseStatus(JSON.stringify({...ready,accepted:false}));
  assert.equal(status.enabled[1],false);
  assert.equal(status.accepted,false);
});
test('Player and Event use disjoint feature IDs and integer tuning values', () => {
  assert.equal(PLAYER.some(p => EVENT.some(e => e.id === p.id)),false);
  assert.equal(bounded(2.5,2,50),false);
  assert.equal(bounded(NaN,2,50),false);
  assert.equal(bounded(50,2,50),true);
});
