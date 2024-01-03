#include "include/analysis/costAnalysis.h"
namespace COST {
double linearInterpolation(double x, std::vector<long long> &xData,
                           std::vector<double> &yData) {
  long long n = xData.size();
  long long i = 0;
  while (i < n && xData[i] < x) {
    ++i;
  }
  if (i == 0) {
    double x0 = xData[0];
    double x1 = xData[1];
    double y0 = yData[0];
    double y1 = yData[1];
    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
  } else if (i == n) {
    double x0 = xData[n - 1];
    double x1 = xData[n - 2];
    double y0 = yData[n - 1];
    double y1 = yData[n - 2];
    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
  } else {
    double x0 = xData[i - 1];
    double x1 = xData[i];
    double y0 = yData[i - 1];
    double y1 = yData[i];
    return y0 + (y1 - y0) * (x - x0) / (x1 - x0);
  }
}
COSTDADA::COSTDADA() {
  _sramData.addItem(64, 0.2028, 0.3597, 452.059, 0.0988);
  _sramData.addItem(128, 0.3288, 0.4344, 651.6205, 0.1495);
  _sramData.addItem(256, 0.5516, 0.5888, 1148.8453, 0.259);
  _sramData.addItem(512, 0.7784, 0.8672, 2127.9993, 0.49);
  _sramData.addItem(1024, 1.2904, 1.1153, 3785.8146, 0.8914);
  _sramData.addItem(2048, 2.2996, 1.5964, 8068.5242, 2.0497);
  _sramData.addItem(4096, 4.2426, 2.4835, 16401.2159, 3.8893);
  _sramData.addItem(8192, 6.2515, 2.8162, 29693.9656, 7.5318);
  _sramData.addItem(16384, 12.0808, 4.4999, 57950.0339, 13.7307);
  _sramData.addItem(32768, 13.8107, 6.2299, 119517.0368, 27.4613);
  _sramData.addItem(65536, 17.224, 9.5805, 229131.0173, 53.6437);
  _sramData.addItem(131072, 23.0602, 14.8217, 483380.391, 99.9686);
  _sramData.addItem(262144, 30.6722, 22.4338, 971038.4994, 199.937);

  _regData.addItem(1, 0.0557, 0.0578, 105.754, 0.0109);
  _regData.addItem(2, 0.1159, 0.1225, 218.6225, 0.0233);
  _regData.addItem(4, 0.2291, 0.2376, 465.5285, 0.0458);
  _regData.addItem(8, 0.4532, 0.4622, 934.156, 0.0918);
  _regData.addItem(16, 0.9011, 0.9105, 1864.006, 0.1839);
  _regData.addItem(32, 1.7979, 1.8071, 3726.649, 0.3785);
  _regData.addItem(64, 3.5933, 3.5998, 7649.2085, 0.7836);
  _regData.addItem(128, 7.1914, 7.1854, 15345.123, 1.5634);
  _regData.addItem(256, 14.3831, 14.3581, 30728.7345, 3.1061);
  _regData.addItem(512, 28.769, 28.7075, 61582.7045, 6.047);
  _regData.addItem(1024, 57.5233, 57.3984, 123253.3815, 11.9185);
  _regData.addItem(2048, 114.9812, 114.7555, 246587.4895, 23.6135);

  _systolicInput.addItem(8, 2, 0.00372, 42.03431, 0.0001);
  _systolicInput.addItem(8, 4, 0.00743, 84.06863, 0.0002);
  _systolicInput.addItem(8, 8, 0.01486, 168.13726, 0.0004);
  _systolicInput.addItem(8, 16, 0.02972, 336.27452, 0.0008);
  _systolicInput.addItem(8, 32, 0.05944, 672.54904, 0.0016);
  _systolicInput.addItem(8, 64, 0.11889, 1345.09807, 0.00319);
  _systolicInput.addItem(8, 128, 0.23777, 2690.19614, 0.00639);
  _systolicInput.addItem(16, 2, 0.00702, 79.39815, 0.00019);
  _systolicInput.addItem(16, 4, 0.01404, 158.7963, 0.00038);
  _systolicInput.addItem(16, 8, 0.02807, 317.5926, 0.00075);
  _systolicInput.addItem(16, 16, 0.05614, 635.1852, 0.00151);
  _systolicInput.addItem(16, 32, 0.11228, 1270.3704, 0.00302);
  _systolicInput.addItem(16, 64, 0.22456, 2540.7408, 0.00603);
  _systolicInput.addItem(16, 128, 0.44911, 5081.48161, 0.01206);
  _systolicInput.addItem(32, 2, 0.01362, 154.12582, 0.00037);
  _systolicInput.addItem(32, 4, 0.02724, 308.25164, 0.00073);
  _systolicInput.addItem(32, 8, 0.05449, 616.50328, 0.00146);
  _systolicInput.addItem(32, 16, 0.10898, 1233.00657, 0.00293);
  _systolicInput.addItem(32, 32, 0.21796, 2466.01313, 0.00585);
  _systolicInput.addItem(32, 64, 0.43591, 4932.02626, 0.01171);
  _systolicInput.addItem(32, 128, 0.8718, 9864.05253, 0.02341);

  _systolicOutput.addItem(8, 2, 0.01474, 93.97224, 0.0013);
  _systolicOutput.addItem(8, 4, 0.01805, 131.33608, 0.00139);
  _systolicOutput.addItem(8, 8, 0.02465, 206.06375, 0.00157);
  _systolicOutput.addItem(8, 16, 0.03786, 355.51909, 0.00192);
  _systolicOutput.addItem(8, 32, 0.06428, 654.42977, 0.00263);
  _systolicOutput.addItem(8, 64, 0.11712, 1252.25114, 0.00405);
  _systolicOutput.addItem(8, 128, 0.22279, 2447.89387, 0.00689);
  _systolicOutput.addItem(16, 2, 0.03593, 229.00312, 0.00325);
  _systolicOutput.addItem(16, 4, 0.04254, 303.73079, 0.00343);
  _systolicOutput.addItem(16, 8, 0.05575, 453.18613, 0.00378);
  _systolicOutput.addItem(16, 16, 0.08217, 752.09681, 0.00449);
  _systolicOutput.addItem(16, 32, 0.13501, 1349.91818, 0.00591);
  _systolicOutput.addItem(16, 64, 0.24023, 2541.23866, 0.00874);
  _systolicOutput.addItem(16, 128, 0.45157, 4932.52412, 0.01441);
  _systolicOutput.addItem(32, 2, 0.1237, 848.90775, 0.01362);
  _systolicOutput.addItem(32, 4, 0.13088, 973.86758, 0.01477);
  _systolicOutput.addItem(32, 8, 0.1634, 1297.1458, 0.01467);
  _systolicOutput.addItem(32, 16, 0.2154, 1896.23969, 0.01573);
  _systolicOutput.addItem(32, 32, 0.32075, 3096.62391, 0.01806);
  _systolicOutput.addItem(32, 64, 0.52696, 5462.2675, 0.02222);
  _systolicOutput.addItem(32, 128, 0.95479, 10270.48029, 0.03509);

  _multicastOutput.addItem(8, 2, 0.155, 271.4138, 0.00169);
  _multicastOutput.addItem(8, 4, 0.1863, 386.19925, 0.00363);
  _multicastOutput.addItem(8, 8, 0.28928, 565.89852, 0.00586);
  _multicastOutput.addItem(8, 16, 0.53904, 928.42633, 0.00803);
  _multicastOutput.addItem(8, 32, 1.05545, 1660.29122, 0.01315);
  _multicastOutput.addItem(8, 64, 2.11458, 3104.58389, 0.02485);
  _multicastOutput.addItem(8, 128, 4.16045, 6274.08946, 0.0542);
  _multicastOutput.addItem(16, 2, 0.31322, 583.4556, 0.00377);
  _multicastOutput.addItem(16, 4, 0.39566, 801.52684, 0.0076);
  _multicastOutput.addItem(16, 8, 0.61507, 1155.37261, 0.01009);
  _multicastOutput.addItem(16, 16, 1.1574, 1856.70236, 0.01456);
  _multicastOutput.addItem(16, 32, 2.25921, 3263.62968, 0.02096);
  _multicastOutput.addItem(16, 64, 4.50869, 6259.16738, 0.04021);
  _multicastOutput.addItem(16, 128, 8.99957, 12361.05259, 0.06902);
  _multicastOutput.addItem(32, 2, 0.73209, 1808.32592, 0.01692);
  _multicastOutput.addItem(32, 4, 0.97941, 2623.95489, 0.02941);
  _multicastOutput.addItem(32, 8, 1.37412, 2807.77636, 0.02966);
  _multicastOutput.addItem(32, 16, 2.55408, 4325.72497, 0.0375);
  _multicastOutput.addItem(32, 32, 4.81736, 7361.6557, 0.06046);
  _multicastOutput.addItem(32, 64, 9.47842, 13181.95912, 0.06414);
  _multicastOutput.addItem(32, 128, 18.58481, 25257.67061, 0.1461);

  _multicastInput.addItem(8, 2, 0.01081, 79.58706, 0.00349);
  _multicastInput.addItem(8, 4, 0.01804, 134.49827, 0.00581);
  _multicastInput.addItem(8, 8, 0.03132, 240.1198, 0.00962);
  _multicastInput.addItem(8, 16, 0.06964, 542.37975, 0.02197);
  _multicastInput.addItem(8, 32, 0.12721, 1031.86723, 0.03944);
  _multicastInput.addItem(8, 64, 0.15868, 1460.3569, 0.057);
  _multicastInput.addItem(8, 128, 0.30914, 2855.92998, 0.13415);
  _multicastInput.addItem(16, 2, 0.02162, 159.17413, 0.00697);
  _multicastInput.addItem(16, 4, 0.03609, 268.99654, 0.01162);
  _multicastInput.addItem(16, 8, 0.07347, 555.33988, 0.02259);
  _multicastInput.addItem(16, 16, 0.14026, 1084.10893, 0.04437);
  _multicastInput.addItem(16, 32, 0.19109, 1743.91601, 0.05559);
  _multicastInput.addItem(16, 64, 0.31036, 2902.354, 0.09046);
  _multicastInput.addItem(16, 128, 0.57453, 5469.53691, 0.15253);
  _multicastInput.addItem(32, 2, 0.04324, 318.34826, 0.01394);
  _multicastInput.addItem(32, 4, 0.07907, 584.92002, 0.02542);
  _multicastInput.addItem(32, 8, 0.11215, 943.6835, 0.03281);
  _multicastInput.addItem(32, 16, 0.20395, 1820.27968, 0.06026);
  _multicastInput.addItem(32, 32, 0.30708, 3120.00887, 0.08294);
  _multicastInput.addItem(32, 64, 0.53532, 5288.08819, 0.25167);
  _multicastInput.addItem(32, 128, 1.08388, 10525.14303, 0.46679);

  _unicastInput.addItem(8, 1, 0.00186, 21.01716, 0.00005);
  _unicastInput.addItem(16, 1, 0.00351, 39.69908, 0.00009);
  _unicastInput.addItem(32, 1, 0.00681, 77.06291, 0.00018);

  _unicastOutput.addItem(8, 1, 0.01309, 75.29032, 0.00126);
  _unicastOutput.addItem(16, 1, 0.03263, 191.63928, 0.00316);
  _unicastOutput.addItem(32, 1, 0.11102, 751.76891, 0.01202);

  _mac.addItem(8, 1, 0.19867, 667.02782, 0.01466);
  _mac.addItem(16, 1, 0.64706, 2035.63349, 0.03746);
  _mac.addItem(32, 1, 2.42793, 7918.4507, 0.08396);
}

} // namespace COST
COST::COSTDADA _Cost;