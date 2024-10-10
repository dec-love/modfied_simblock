import pandas as pd
import matplotlib.pyplot as plt
import glob
import numpy as np

# CSVファイルのパターンを指定
file_pattern = "simulation_results_T600.csv"

def generate_hashrates(N, s):
    hashrates = []
    normalizing_constant = 0.0
    
    # 正規化定数の計算
    for n in range(1, N + 1):
        normalizing_constant += 1.0 / np.power(n, s)

    # 各ハッシュレートの計算
    for k in range(1, N + 1):
        hashrate_value = (1.0 / np.power(k, s)) / normalizing_constant
        hashrates.append(hashrate_value)
    
    return hashrates


# グラフの作成
plt.figure(figsize=(10, 6))

# ファイルを読み込んでプロット
for file in glob.glob(file_pattern):
    # CSVファイルを読み込み
    df = pd.read_csv(file)
    
    # Tの値をファイル名から抽出
    # T_value = file.split('_')[-1].split('.')[0]
    if file.split('T')[-1].split('.')[0][0] == '0':
        T_value = file.split('T')[-1].split('.')[0]
        # 1文字目と2文字目の間に.を追加
        T_value = T_value[:1] + '.' + T_value[1:]
        # print(T_value)
    else:
        T_value = file.split('T')[-1].split('.')[0]
    # T_valueをdoubleにする
    T_value = float(T_value)

    
    # real_stale_rateのプロット
    plt.plot(df['s'], df['real_staleRate'], linestyle='None', marker='x', label=f'Simulated stale block rate')
    plt.plot(df['s'], df['theoretical_staleRate'], linestyle='None', marker='o', label=f'Proposed theoretical stale block rate')
    
    # sの値ごとに df['real_staleRate']に対するdf['theoretical_staleRate']の誤差を計算し，printする
    errorList = []
    for i in range(len(df['s'])):
        error = abs((df['real_staleRate'][i] - df['theoretical_staleRate'][i]) / df['real_staleRate'][i] * 100)
        errorList.append(error)
    print(sum(errorList)/len(errorList))
    
    errorList2 = []
    for i in range(len(df['s'])):
        error = abs(((1/T_value)*df['maxPropTime'][i]/10**3 - df['real_staleRate'][i]) / df['real_staleRate'][i] * 100)
        errorList2.append(error)
    print(sum(errorList2)/len(errorList2))

    # グラフの線を結ばないようにする
    plt.plot(df['s'], (1/T_value)*df['maxPropTime']/10**3, 
        label=f'Conventional theoretical stale block rate', 
        linestyle='None', marker='o')
    hhiList = []
    for s in np.arange(0, 2.01, 0.05):
        hashrateList = generate_hashrates(20, s)
        hhi = 0
        for i in range(len(hashrateList)):
            hhi += hashrateList[i]**2
        hhiList.append(hhi)
            

plt.xlabel('s', fontsize=22)
plt.ylabel('Stale block rate', fontsize=20)
plt.tick_params(axis='both', which='major', labelsize=14)
plt.legend(fontsize=16)
plt.grid(True)
plt.tight_layout()
# グラフの表示
plt.show()

