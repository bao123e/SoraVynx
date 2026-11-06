/* SPDX-License-Identifier: AGPL-3.0-or-later */
import React, {useCallback, useEffect, useRef, useState} from 'react';
import {
  ActivityIndicator, Alert, AppState, Image, NativeModules, Pressable,
  ScrollView, StatusBar, StyleSheet, Switch, Text, useWindowDimensions, View,
} from 'react-native';
import {SafeAreaProvider, SafeAreaView} from 'react-native-safe-area-context';
import {MenuView} from '@react-native-menu/menu';
import {canToggle, describe, EVENT, PLAYER, parseStatus} from './src/model';
import type {RuntimeStatus, Tab} from './src/model';

type Appearance = {image?: string | null; imageOpacity: number; panelOpacity: number; buttonOpacity: number};
type OpacityKey = 'imageOpacity' | 'panelOpacity' | 'buttonOpacity';
const bridge = NativeModules.SoraVynxGame as {
  getStatus(): Promise<string>; command(kind: number, value: number): Promise<string>;
  launchGame(): Promise<void>; appearance(): Promise<string>;
  setOpacity(key: string, value: number): Promise<void>;
  pickBackground(): Promise<string | null>; removeBackground(): Promise<void>;
};
const mint = '#5EEAD4';
const tabs: Tab[] = ['Player', 'Event', 'Custom BG'];
const defaults: Appearance = {imageOpacity: 60, panelOpacity: 85, buttonOpacity: 85};
const rgba = (hex: string, opacity: number) => `${hex}${Math.round(opacity * 2.55).toString(16).padStart(2, '0')}`;

function ControlPanel() {
  const [tab, setTab] = useState<Tab>('Player');
  const [status, setStatus] = useState<RuntimeStatus>({phase: -1});
  const [appearance, setAppearance] = useState<Appearance>(defaults);
  const [busy, setBusy] = useState(false);
  const busyRef = useRef(false);
  const pollingRef = useRef(false);
  const generation = useRef(0);
  const [error, setError] = useState('');
  const {width} = useWindowDimensions();
  const panelColor = rgba('#102034', appearance.panelOpacity);
  const buttonColor = rgba('#24394D', appearance.buttonOpacity);
  const refresh = useCallback(async () => {
    if (busyRef.current || pollingRef.current) {return;}
    pollingRef.current = true;
    const request = generation.current;
    try {
      const next = parseStatus(await bridge.getStatus());
      if (request === generation.current) {setStatus(old => ({...next, enabled: next.enabled ?? old.enabled}));}
    } catch {if (request === generation.current) {setStatus(old => ({phase: -1, enabled: old.enabled}));}}
    finally {pollingRef.current = false;}
  }, []);
  useEffect(() => {
    let mounted = true;
    bridge.appearance().then(value => {
      const saved = JSON.parse(value) as Appearance;
      if (mounted) {setAppearance({...defaults, ...saved});}
    }).catch(() => {});
    void refresh();
    const interval = setInterval(() => {if (AppState.currentState === 'active') {void refresh();}}, 2500);
    const listener = AppState.addEventListener('change', state => {if (state === 'active') {void refresh();}});
    return () => {mounted = false; clearInterval(interval); listener.remove();};
  }, [refresh]);

  async function command(kind: number, value: number) {
    if (busyRef.current) {return;}
    busyRef.current = true; generation.current++; setBusy(true); setError('');
    try {
      const next = parseStatus(await bridge.command(kind, value));
      setStatus(old => ({...next, enabled: next.enabled ?? old.enabled}));
      if (next.accepted === false) {setError('Backend từ chối thay đổi. Xem trạng thái chức năng gần nhất.');}
      else if (next.accepted === undefined) {setError('Chưa xác nhận thay đổi. Kiểm tra lại khi game phản hồi.');}
    } catch {setError('Mất kết nối với game. Mở lại game rồi kiểm tra trạng thái.');}
    finally {busyRef.current = false; setBusy(false);}
  }
  async function launch() {
    try {await bridge.launchGame();}
    catch {Alert.alert('Chưa mở được game', 'Cài cả menu và game tích hợp từ cùng bộ build. Không thể điều khiển bản game gốc.');}
  }
  async function opacity(key: OpacityKey, value: number) {
    const clamped = Math.max(0, Math.min(100, value));
    try {await bridge.setOpacity(key, clamped); setAppearance(old => ({...old, [key]: clamped}));}
    catch {setError('Không lưu được độ trong suốt.');}
  }
  function button(label: string, action: () => void, disabled = false) {
    return <Pressable accessibilityRole="button" disabled={disabled} onPress={action}
      style={({pressed}) => [styles.button, {backgroundColor: buttonColor}, (disabled || pressed) && styles.dim]}>
      <Text style={styles.buttonText}>{label}</Text>
    </Pressable>;
  }
  function multiplier(label: string, kind: number, value: number, min: number, max: number) {
    return <View style={styles.setting}>
      <Text style={styles.settingLabel}>{label}</Text>
      <View style={styles.stepper}>
        {button('−', () => {void command(kind, value - 1);}, busy || status.phase !== 3 || value <= min)}
        <Text accessibilityLiveRegion="polite" style={styles.number}>{value}</Text>
        {button('+', () => {void command(kind, value + 1);}, busy || status.phase !== 3 || value >= max)}
      </View>
      <Text style={styles.muted}>{min}–{max}</Text>
    </View>;
  }
  const features = tab === 'Event' ? EVENT : PLAYER;
  return <SafeAreaView style={styles.root}>
    <StatusBar barStyle="light-content" backgroundColor="#09111E" />
    {appearance.image ? <View pointerEvents="none" style={StyleSheet.absoluteFill}><Image source={{uri: appearance.image}}
      resizeMode="contain" style={[StyleSheet.absoluteFill, {opacity: appearance.imageOpacity / 100}]} /></View> : null}
    <View style={styles.header}>
      <View style={styles.brand}>
        <Image accessible={false} source={require('./assets/brand.png')} style={styles.logo} resizeMode="contain" />
        <View style={styles.brandText}><Text style={styles.title}>SoraVynx</Text><Text style={styles.eyebrow}>STELLA SORA · ANDROID</Text></View>
      </View>
      <MenuView isAnchoredToRight shouldOpenOnLongPress={false}
        actions={[{id: 'game', title: 'Mở game'}, {id: 'refresh', title: 'Kiểm tra kết nối'}, {id: 'off', title: 'Tắt tất cả'}, {id: 'about', title: 'Thông tin bản thử nghiệm'}]}
        onPressAction={({nativeEvent}) => {
          if (nativeEvent.event === 'game') {void launch();}
          else if (nativeEvent.event === 'refresh') {void refresh();}
          else if (nativeEvent.event === 'off') {void command(-1, 0);}
          else {Alert.alert('SoraVynx · Preview', 'Menu dùng @react-native-menu/menu. Backend ARM64 chỉ dành cho bản game tích hợp 2.0.0. Hiệu quả Player/Event chưa được xác minh trên điện thoại. Không có Auto Daily hoặc ghi log.');}
        }}>
        <View accessibilityRole="button" accessibilityLabel="Mở menu" style={[styles.button, {backgroundColor: buttonColor}]}><Text style={styles.buttonText}>Menu</Text></View>
      </MenuView>
    </View>
    <ScrollView horizontal showsHorizontalScrollIndicator={false} style={styles.tabs} contentContainerStyle={styles.tabContent}>
      {tabs.map(item => <Pressable key={item} accessibilityRole="tab" accessibilityState={{selected: tab === item}}
        onPress={() => setTab(item)} style={[styles.tab, {backgroundColor: buttonColor}, tab === item && styles.selected]}>
        <Text style={[styles.tabLabel, tab === item && styles.selectedLabel]}>{item}</Text>
      </Pressable>)}
    </ScrollView>
    <ScrollView style={styles.content} contentContainerStyle={styles.contentContainer}>
      <View style={[styles.status, {backgroundColor: panelColor}]}>
        <Text style={styles.statusTitle}>{status.phase === 3 ? 'BACKEND CONNECTED · PREVIEW' : 'GAME CONNECTION'}</Text>
        <Text accessibilityLiveRegion="polite" style={styles.body}>{describe(status)}</Text>
        {busy ? <ActivityIndicator color={mint} accessibilityLabel="Đang gửi thay đổi" /> : null}
        {error ? <Text accessibilityRole="alert" style={styles.error}>{error}</Text> : null}
        {status.phase !== 3 ? button('Mở game', () => {void launch();}) : null}
      </View>
      {tab === 'Custom BG' ? <View style={[styles.panel, {backgroundColor: panelColor}]}>
        <Text style={styles.heading}>Custom Background</Text>
        <Text style={styles.body}>Ảnh phủ toàn giao diện · giữ nguyên tỉ lệ, không cắt ảnh.</Text>
        {button(appearance.image ? 'Đổi ảnh…' : 'Chọn ảnh…', () => {
          bridge.pickBackground().then(image => {if (image) {setAppearance(old => ({...old, image}));}})
            .catch(() => setError('Không mở được ảnh đã chọn.'));
        })}
        {appearance.image ? button('Xóa ảnh', () => {
          bridge.removeBackground().then(() => setAppearance(old => ({...old, image: null}))).catch(() => setError('Không xóa được ảnh.'));
        }) : null}
        {([['imageOpacity', 'Opacity ảnh nền'], ['panelOpacity', 'Opacity các khung'], ['buttonOpacity', 'Opacity nút bấm']] as const).map(([key, label]) =>
          <View key={key} style={styles.setting}>
            <Text style={styles.settingLabel}>{label}</Text>
            <View style={styles.stepper}>
              {button('−', () => {void opacity(key, appearance[key] - 10);}, appearance[key] <= 0)}
              <MenuView shouldOpenOnLongPress={false}
                actions={[0, 25, 50, 75, 100].map(n => ({id: String(n), title: `${n}%`}))}
                onPressAction={({nativeEvent}) => {void opacity(key, Number(nativeEvent.event));}}>
                <View accessibilityRole="button" accessibilityLabel={`${label}: ${appearance[key]} phần trăm, chọn giá trị`} style={styles.percent}>
                  <Text style={styles.number}>{appearance[key]}%</Text>
                </View>
              </MenuView>
              {button('+', () => {void opacity(key, appearance[key] + 10);}, appearance[key] >= 100)}
            </View>
          </View>)}
      </View> : <View style={[styles.columns, width >= 720 && styles.wide]}>
        <View style={[styles.panel, width >= 720 && styles.grow, {backgroundColor: panelColor}]}>
          <Text style={styles.heading}>{tab === 'Player' ? 'Player' : 'Bouncing Bumper Bash'}</Text>
          <Text style={styles.body}>{tab === 'Player' ? 'Combat controls' : 'Event controls · independent of Player'}</Text>
          {features.map(feature => {
            const available = canToggle(status, feature.id);
            const enabled = status.enabled?.[feature.id] === true;
            return <View key={feature.id} style={[styles.feature, {backgroundColor: buttonColor}]}>
              <View style={styles.featureText}><Text style={styles.settingLabel}>{feature.label}</Text>
                <Text style={styles.muted}>{available ? (enabled ? 'ON · kiểm tra trong game' : 'OFF') : status.enabled ? 'Trạng thái gần nhất · chưa nối được' : 'Chưa khả dụng'}</Text>
              </View>
              <Switch accessibilityLabel={feature.label} accessibilityHint={available ? 'Thay đổi chức năng trong game tích hợp' : 'Backend chưa nối được chức năng này'}
                style={styles.switch} value={enabled} disabled={!available || busy} onValueChange={value => {void command(feature.id, value ? 1 : 0);}}
                trackColor={{false: '#52647B', true: '#257A70'}} thumbColor={enabled ? mint : '#D5DEEC'} />
            </View>;
          })}
        </View>
        <View style={[styles.panel, width >= 720 && styles.grow, {backgroundColor: panelColor}]}>
          <Text style={styles.heading}>Tuning</Text>
          {tab === 'Player' ? <>
            {multiplier('Damage multiplier', 10, status.damage ?? 2, 2, 50)}
            {multiplier('Vacuum distance', 11, status.vacuum ?? 3, 1, 10)}
          </> : multiplier('Score multiplier', 12, status.score ?? 2, 2, 100)}
          <Text style={styles.body}>Chỉ bật được khi backend tìm thấy hàm tương thích. Bản thử nghiệm chưa xác minh hiệu quả trên thiết bị thật.</Text>
        </View>
      </View>}
      <View style={styles.footer}>
        {button('Tắt tất cả', () => {void command(-1, 0);}, busy)}
        {button('Về game', () => {void launch();})}
      </View>
      <Text style={styles.footnote}>ARM64 · 2.0.0 · Preview 0.1.0</Text>
    </ScrollView>
  </SafeAreaView>;
}

export default function App() {return <SafeAreaProvider><ControlPanel /></SafeAreaProvider>;}
const styles = StyleSheet.create({
  root: {flex: 1, backgroundColor: '#09111E'},
  header: {padding: 20, flexDirection: 'row', alignItems: 'center', justifyContent: 'space-between', gap: 12},
  brand: {flexDirection: 'row', alignItems: 'center', gap: 10, flexShrink: 1},
  brandText: {flexShrink: 1}, logo: {width: 36, height: 42}, title: {fontFamily: 'BeVietnamPro-Bold', fontSize: 27, color: mint},
  eyebrow: {fontFamily: 'BeVietnamPro-Regular', fontSize: 10, color: '#D5DEEC', letterSpacing: 1},
  tabs: {flexGrow: 0}, tabContent: {paddingHorizontal: 20, gap: 8, paddingBottom: 14},
  tab: {minHeight: 48, paddingHorizontal: 20, paddingVertical: 13, borderRadius: 12, borderWidth: 1, borderColor: '#6A7E94'},
  selected: {borderColor: mint}, tabLabel: {fontFamily: 'BeVietnamPro-SemiBold', color: '#F5F8FF', fontSize: 15}, selectedLabel: {color: mint},
  content: {flex: 1}, contentContainer: {padding: 20, paddingTop: 0, gap: 16},
  status: {borderRadius: 16, padding: 16, gap: 10}, statusTitle: {fontFamily: 'BeVietnamPro-SemiBold', color: mint, fontSize: 12, letterSpacing: 1},
  body: {fontFamily: 'BeVietnamPro-Regular', color: '#E5ECF7', fontSize: 14, lineHeight: 23},
  heading: {fontFamily: 'BeVietnamPro-Bold', color: '#F5F8FF', fontSize: 21},
  panel: {borderRadius: 16, padding: 16, gap: 14}, columns: {gap: 16}, wide: {flexDirection: 'row', alignItems: 'flex-start'}, grow: {flex: 1},
  feature: {borderRadius: 12, minHeight: 72, padding: 14, flexDirection: 'row', alignItems: 'center', gap: 8},
  featureText: {flex: 1, gap: 5}, switch: {minHeight: 48, minWidth: 52}, settingLabel: {fontFamily: 'BeVietnamPro-SemiBold', color: '#F5F8FF', fontSize: 15, flexShrink: 1},
  muted: {fontFamily: 'BeVietnamPro-Regular', color: '#CDD8E8', fontSize: 12, lineHeight: 20},
  button: {minHeight: 48, minWidth: 48, justifyContent: 'center', alignItems: 'center', borderRadius: 10, paddingHorizontal: 14, paddingVertical: 12},
  buttonText: {fontFamily: 'BeVietnamPro-SemiBold', color: '#F5F8FF', fontSize: 15}, dim: {opacity: 0.5},
  setting: {gap: 10, paddingVertical: 8}, stepper: {flexDirection: 'row', gap: 12, alignItems: 'center'},
  number: {fontFamily: 'BeVietnamPro-SemiBold', color: mint, fontSize: 18, minWidth: 48, textAlign: 'center'},
  percent: {minHeight: 48, minWidth: 86, justifyContent: 'center'},
  footer: {flexDirection: 'row', gap: 12, flexWrap: 'wrap'},
  footnote: {fontFamily: 'BeVietnamPro-Regular', color: '#CDD8E8', fontSize: 12},
  error: {fontFamily: 'BeVietnamPro-Regular', color: '#FFB7B7', fontSize: 14, lineHeight: 22},
});
