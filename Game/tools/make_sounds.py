# 効果音を合成して 16bit / 44.1kHz / モノラルの wav に書き出す（標準ライブラリだけ）
# 使い方（リポジトリの直下で）: python Game/tools/make_sounds.py Game/resources/sounds
# 音を変えたいときは下の関数の数値（周波数・長さ・減衰の速さ）を変えて、もう一度実行する
# swing.wav（近接の空振り）と throw.wav（ブーメランを投げる）は合成ではなく、Kenney「RPG Audio」（CC0）の
# knifeSlice2.ogg ／ knifeSlice.ogg を wav にして、頭と終わりの無音を削ったもの（このスクリプトでは作らない）
import math
import random
import struct
import sys
import wave
from pathlib import Path

RATE = 44100
random.seed(20261027)


def n_samples(seconds):
    return int(RATE * seconds)


def env_ad(i, n, attack=0.005, curve=3.0):
    """短いアタックのあと指数的に減衰する包絡線"""
    t = i / RATE
    a = min(1.0, t / attack) if attack > 0 else 1.0
    d = math.exp(-curve * i / n)
    # 末尾は 0 へ（クリック防止）
    tail = min(1.0, (n - i) / (RATE * 0.004))
    return a * d * tail


class OnePole:
    """1 次のローパス（cutoff は毎サンプル変えられる）"""
    def __init__(self):
        self.y = 0.0

    def lp(self, x, cutoff):
        a = 1.0 - math.exp(-2.0 * math.pi * cutoff / RATE)
        self.y += a * (x - self.y)
        return self.y


def band_noise(n, low_hz, high_hz, sweep=None):
    """2 つのローパスの差でバンドパス風のノイズ（sweep(t)->(low, high) で周波数を動かせる）"""
    lp1, lp2 = OnePole(), OnePole()
    out = []
    for i in range(n):
        lo, hi = (low_hz, high_hz) if sweep is None else sweep(i / n)
        x = random.uniform(-1.0, 1.0)
        out.append(lp1.lp(x, hi) - lp2.lp(x, lo))
    return out


def sine_sweep(n, f0, f1, shape="sine"):
    out = []
    phase = 0.0
    for i in range(n):
        f = f0 * (f1 / f0) ** (i / n)
        phase += 2.0 * math.pi * f / RATE
        if shape == "sine":
            v = math.sin(phase)
        elif shape == "square":
            v = 1.0 if math.sin(phase) >= 0 else -1.0
        elif shape == "triangle":
            v = 2.0 / math.pi * math.asin(math.sin(phase))
        elif shape == "saw":
            v = 2.0 * ((phase / (2.0 * math.pi)) % 1.0) - 1.0
        out.append(v)
    return out


def lowpass(track, cutoff):
    """1 次のローパスを掛ける（耳に痛い高い成分を丸める）"""
    lp = OnePole()
    return [lp.lp(v, cutoff) for v in track]


def swell(n, rise=0.8):
    """だんだん大きくなり、最後の (1 - rise) の割合で素早く消える包絡線"""
    out = []
    for i in range(n):
        t = i / n
        out.append((t / rise) ** 1.5 if t < rise else max(0.0, 1.0 - (t - rise) / (1.0 - rise)))
    return out


def mix(*tracks):
    n = max(len(t) for t in tracks)
    return [sum(t[i] for t in tracks if i < len(t)) for i in range(n)]


def scale(track, gain):
    return [v * gain for v in track]


def apply_env(track, attack=0.005, curve=3.0):
    n = len(track)
    return [v * env_ad(i, n, attack, curve) for i, v in enumerate(track)]


def soft_clip(track, drive=1.0):
    return [math.tanh(v * drive) for v in track]


def normalize(track, peak=0.85):
    m = max(abs(v) for v in track) or 1.0
    return [v * peak / m for v in track]


def write(path, track):
    track = normalize(track)
    with wave.open(str(path), "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1.0, min(1.0, v)) * 32767)) for v in track))


# ---------------------------------------------------------------- 各効果音

def jump():
    # 軽く上がる「ピョン」：三角波を 280→720Hz へ上げる
    n = n_samples(0.14)
    tone = sine_sweep(n, 280, 720, "triangle")
    return apply_env(tone, attack=0.003, curve=4.0)


def swing():
    # 近接の空振り「ブンッ」：ノイズの帯域を高→低へ動かす
    n = n_samples(0.16)
    noise = band_noise(n, 0, 0, sweep=lambda t: (300 + 900 * (1 - t), 2500 + 4000 * (1 - t)))
    env = [math.sin(math.pi * min(1.0, i / n)) ** 1.5 for i in range(n)]
    return [v * e for v, e in zip(noise, env)]


def throw():
    # ブーメランを投げる「ヒュンッ」：ノイズの帯域を低→高へ動かし、細い音を重ねる
    n = n_samples(0.22)
    noise = band_noise(n, 0, 0, sweep=lambda t: (400 + 1500 * t, 1500 + 5000 * t))
    env = [math.sin(math.pi * min(1.0, i / n)) for i in range(n)]
    whoosh = [v * e for v, e in zip(noise, env)]
    whistle = apply_env(sine_sweep(n, 900, 1600), attack=0.03, curve=3.0)
    return mix(whoosh, scale(whistle, 0.15))


def catch():
    # 受け取る「パシッ」：短いノイズのクリックと高めの 2 音
    n = n_samples(0.10)
    click = apply_env(band_noise(n_samples(0.02), 2000, 9000), attack=0.0005, curve=6.0)
    blip1 = apply_env(sine_sweep(n_samples(0.05), 1046, 1046, "triangle"), attack=0.001, curve=5.0)
    blip2 = [0.0] * n_samples(0.04) + apply_env(sine_sweep(n_samples(0.06), 1568, 1568, "triangle"), attack=0.001, curve=5.0)
    return mix(scale(click, 0.8), scale(blip1, 0.5), scale(blip2, 0.5))


def hit():
    # ボスへのヒット「ドッ」：低い音の落ち込みとノイズの破裂
    n = n_samples(0.16)
    thump = apply_env(sine_sweep(n, 180, 55), attack=0.001, curve=5.0)
    burst = apply_env(band_noise(n_samples(0.06), 300, 5000), attack=0.0005, curve=5.0)
    return soft_clip(mix(thump, scale(burst, 0.9)), 1.8)


def core_hit():
    # コアへのヒット「ガキンッ」：ヒットに高めの金属の響きを足す
    n = n_samples(0.32)
    base = hit()
    ring = [0.0] * n
    for f, a, c in ((1320, 0.5, 9.0), (1980, 0.35, 11.0), (2770, 0.25, 13.0)):
        tone = apply_env(sine_sweep(n, f, f), attack=0.001, curve=c)
        ring = mix(ring, scale(tone, a))
    return mix(base, scale(ring, 0.8))


def guard():
    # 盾で弾かれる「カンッ」：倍音が整数倍でない金属音と、硬いクリック
    n = n_samples(0.40)
    clang = [0.0] * n
    for f, a, c in ((620, 0.6, 7.0), (1530, 0.45, 8.0), (2410, 0.35, 10.0), (3370, 0.25, 12.0), (4710, 0.15, 14.0)):
        tone = apply_env(sine_sweep(n, f, f * 0.995), attack=0.0008, curve=c)
        clang = mix(clang, scale(tone, a))
    click = apply_env(band_noise(n_samples(0.015), 3000, 12000), attack=0.0003, curve=4.0)
    return mix(clang, scale(click, 0.7))


def damage():
    # プレイヤーの被弾「ビシッ」：下がる矩形波とノイズ
    n = n_samples(0.26)
    tone = apply_env(sine_sweep(n, 700, 160, "square"), attack=0.002, curve=3.5)
    noise = apply_env(band_noise(n_samples(0.10), 500, 6000), attack=0.001, curve=4.0)
    return soft_clip(mix(scale(tone, 0.5), scale(noise, 0.7)), 1.5)


def land():
    # ボスが床に落ちる「ズシン」：とても低い音の落ち込みと、低くこもった地響き
    n = n_samples(0.70)
    thump = apply_env(sine_sweep(n, 95, 35), attack=0.002, curve=4.5)
    rumble = apply_env(band_noise(n, 20, 260), attack=0.003, curve=3.5)
    impact = apply_env(band_noise(n_samples(0.05), 200, 2500), attack=0.0005, curve=5.0)
    return soft_clip(mix(thump, scale(rumble, 1.6), scale(impact, 0.6)), 2.0)


def charge_windup():
    # 突進の予告「ブォォン↑」：低いのこぎり波を 70→170Hz へ上げながら大きくする（予備動作 0.75 秒に合わせる）
    n = n_samples(0.75)
    engine = lowpass(sine_sweep(n, 70, 170, "saw"), 900)
    growl = band_noise(n, 60, 500)
    env = swell(n, 0.85)
    return soft_clip([(e + 0.5 * g) * a for e, g, a in zip(engine, growl, env)], 2.0)


def slam_windup():
    # 叩きつけの予告「ゴォォ…↑」：低い音とノイズの帯域を上げながら溜める（振りかぶり 1.5 秒のうち前半 1 秒）
    n = n_samples(1.0)
    tone = lowpass(sine_sweep(n, 55, 110, "saw"), 600)
    air = band_noise(n, 0, 0, sweep=lambda t: (150 + 600 * t, 600 + 2500 * t))
    env = swell(n, 0.9)
    return soft_clip([(0.7 * a + 0.6 * b) * e for a, b, e in zip(tone, air, env)], 1.8)


def roar():
    # 落石の予告（吠える）「グォォ」：揺れる低いのこぎり波とノイズ。ふくらんでから消える
    n = n_samples(1.0)
    out = []
    phase = 0.0
    growl = band_noise(n, 80, 900)
    lp = OnePole()
    for i in range(n):
        t = i / n
        f = 85 + 25 * math.sin(2 * math.pi * 7 * i / RATE) - 20 * t  # 細かく揺れながら少し下がる
        phase += 2 * math.pi * f / RATE
        saw = 2.0 * ((phase / (2 * math.pi)) % 1.0) - 1.0
        env = math.sin(math.pi * min(1.0, t * 1.15)) ** 0.7 if t < 1 / 1.15 else 0.0
        out.append(lp.lp(saw + 0.8 * growl[i], 1100) * env)
    return soft_clip(out, 2.5)


def crush_windup():
    # 押しつぶしの予告「キュイーン↑」：しゃがんで跳ぶ合図。音の高さを上げながら細かく揺らす
    n = n_samples(0.7)
    tone = sine_sweep(n, 220, 880, "triangle")
    trem = [0.7 + 0.3 * math.sin(2 * math.pi * 18 * i / RATE) for i in range(n)]
    env = swell(n, 0.8)
    return [t * m * e for t, m, e in zip(tone, trem, env)]


def boss_defeat():
    # ボスの撃破「ドォォン…」：大きな破裂と、長く尾を引く地響き、下がっていく音
    n = n_samples(1.8)
    boom = apply_env(sine_sweep(n, 120, 28), attack=0.002, curve=3.0)
    rumble = apply_env(band_noise(n, 25, 400), attack=0.003, curve=2.2)
    blast = apply_env(band_noise(n_samples(0.35), 150, 4000), attack=0.001, curve=4.0)
    crackle = [0.0] * n
    for k in range(14):
        start = int(n * (0.05 + 0.6 * random.random()))
        pop = apply_env(band_noise(n_samples(0.03), 800, 6000), attack=0.0005, curve=6.0)
        for j, v in enumerate(pop):
            if start + j < n:
                crackle[start + j] += v * (1.0 - start / n) * 0.6
    return soft_clip(mix(boom, scale(rumble, 1.8), scale(blast, 0.9), crackle), 2.2)


def decide():
    # 決定「ピロン」：三角波の 2 音（ミ→シ）
    a = apply_env(sine_sweep(n_samples(0.09), 1318.5, 1318.5, "triangle"), attack=0.002, curve=2.5)
    b = [0.0] * n_samples(0.07) + apply_env(sine_sweep(n_samples(0.22), 1975.5, 1975.5, "triangle"), attack=0.002, curve=3.0)
    return mix(a, scale(b, 0.9))


# 作る順番（乱数を順に使うので、並びを変えると後ろの音が変わる。新しい音は最後に足す）。
# 書き出しが None のものは、Kenney の音に差し替えたので書き出さない（乱数の並びを前と同じにするため、作るだけ作る）
SOUNDS = {
    "jump": jump,
    "swing": (swing, None),
    "throw": (throw, None),
    "catch": catch,
    "hit": hit,
    "coreHit": core_hit,
    "guard": guard,
    "damage": damage,
    "land": land,
    "decide": decide,
    "chargeWindup": charge_windup,
    "slamWindup": slam_windup,
    "roar": roar,
    "crushWindup": crush_windup,
    "bossDefeat": boss_defeat,
}

if __name__ == "__main__":
    out_dir = Path(sys.argv[1])
    out_dir.mkdir(parents=True, exist_ok=True)
    for name, make in SOUNDS.items():
        if isinstance(make, tuple):
            # 書き出さない音も作るだけ作る（乱数の並びを保つ）
            make[0]()
            continue
        track = make()
        write(out_dir / f"{name}.wav", track)
        print(f"{name}.wav  {len(track) / RATE:.2f}s")
