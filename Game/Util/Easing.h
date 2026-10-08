#pragma once

// イージング関数。t は 0〜1 の正規化時間で、戻り値も 0〜1

// 最初が速く、終わりにかけてゆっくりになる（2 次）
float EaseOutQuad(float t);

// 最初がゆっくりで、終わりにかけて速くなる（2 次）
float EaseInQuad(float t);

// 両端がゆっくりで中間が速い（smoothstep）
float EaseInOut(float t);
