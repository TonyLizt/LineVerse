#include "EditDistance.h"

#include <vector>
#include <algorithm>

int EditDistance::levenshtein(const std::string& a, const std::string& b) {
    const int n = static_cast<int>(a.size());
    const int m = static_cast<int>(b.size());

    std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));

    for (int i = 0; i <= n; ++i) dp[i][0] = i;
    for (int j = 0; j <= m; ++j) dp[0][j] = j;

    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            dp[i][j] = std::min({
                dp[i - 1][j] + 1,
                dp[i][j - 1] + 1,
                dp[i - 1][j - 1] + cost
            });
        }
    }

    return dp[n][m];
}

double EditDistance::similarity(const std::string& a, const std::string& b) {
    if (a.empty() && b.empty()) {
        return 1.0;
    }

    int dist = levenshtein(a, b);
    int maxLen = std::max(static_cast<int>(a.size()), static_cast<int>(b.size()));
    return 1.0 - static_cast<double>(dist) / static_cast<double>(maxLen);
}