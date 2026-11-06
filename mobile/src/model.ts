/* SPDX-License-Identifier: AGPL-3.0-or-later */
export type Tab = 'Player' | 'Event' | 'Custom BG';
export type RuntimeStatus = {
  phase: number;
  reason?: string;
  hooks?: number;
  capabilities?: boolean[];
  enabled?: boolean[];
  accepted?: boolean;
  damage?: number;
  vacuum?: number;
  score?: number;
};
export const PLAYER = [
  {id: 1, label: 'God Mode'},
  {id: 2, label: 'Damage Multiplier'},
  {id: 3, label: 'No Cooldown'},
  {id: 4, label: 'Unlimited Energy'},
  {id: 5, label: 'Mob Vacuum'},
] as const;
export const EVENT = [
  {id: 6, label: 'No Cooldown (R)'},
  {id: 7, label: 'Unlimited Energy (R)'},
  {id: 8, label: 'Score Multiplier'},
] as const;

function bitArray(value: unknown): value is number[] {
  return Array.isArray(value) && value.length === 9 && value.every(n => n === 0 || n === 1);
}
export function parseStatus(text: string): RuntimeStatus {
  const raw: unknown = JSON.parse(text);
  if (!raw || typeof raw !== 'object') {throw new Error('INVALID_STATUS');}
  const value = raw as Record<string, unknown>;
  if (!Number.isInteger(value.phase) || Number(value.phase) < -1 || Number(value.phase) > 6) {
    throw new Error('INVALID_PHASE');
  }
  if (value.phase === 3 && (!bitArray(value.capabilities) || !bitArray(value.enabled) ||
      !Number.isInteger(value.hooks) || Number(value.hooks) < 1 || Number(value.hooks) > 24 ||
      !bounded(value.damage, 2, 50) || !bounded(value.vacuum, 1, 10) || !bounded(value.score, 2, 100))) {
    throw new Error('INVALID_READY_STATUS');
  }
  return {
    phase: Number(value.phase), reason: typeof value.reason === 'string' ? value.reason : undefined,
    capabilities: bitArray(value.capabilities) ? value.capabilities.map(Boolean) : undefined,
    enabled: bitArray(value.enabled) ? value.enabled.map(Boolean) : undefined,
    hooks: typeof value.hooks === 'number' ? value.hooks : undefined,
    accepted: typeof value.accepted === 'boolean' ? value.accepted : undefined,
    damage: typeof value.damage === 'number' ? value.damage : undefined,
    vacuum: typeof value.vacuum === 'number' ? value.vacuum : undefined,
    score: typeof value.score === 'number' ? value.score : undefined,
  };
}
export function bounded(n: unknown, min: number, max: number): n is number {
  return typeof n === 'number' && Number.isInteger(n) && n >= min && n <= max;
}
export function canToggle(status: RuntimeStatus, id: number): boolean {
  return status.phase === 3 && id >= 1 && id <= 8 && status.capabilities?.[id] === true;
}
export function describe(status: RuntimeStatus): string {
  if (status.reason === 'ORIGINAL_OR_DIFFERENT_SIGNATURE') {return 'Cần cài game tích hợp cùng bộ APK với menu.';}
  if (status.reason === 'WRONG_GAME_VERSION') {return 'Chỉ hỗ trợ Stella Sora 2.0.0 · ARM64.';}
  if (status.phase === -1) {return 'Chưa nối game. Cài game tích hợp rồi mở game trước.';}
  if (status.phase === 0) {return status.reason === 'GAME_NOT_STARTED' ? 'Mở game để nạp backend.' : 'Đang kiểm tra phiên bản game…';}
  if (status.phase === 1) {return 'Đang chờ runtime game khởi tạo…';}
  if (status.phase === 2) {return 'Đang chờ lớp Player/Event được nạp…';}
  if (status.phase === 3) {return `Đã cài ${status.hooks} hook · cần kiểm tra hiệu quả trong game.`;}
  if (status.phase === 4) {return 'Không tìm được entry tương thích. Chưa có chức năng khả dụng.';}
  return 'Backend không khởi tạo được. Các chức năng giữ tắt.';
}
