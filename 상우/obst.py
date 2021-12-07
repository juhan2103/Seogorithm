
# 출력 표준화
def standard_output(lst):
  n = len(lst)
  for i in range(n):
      for j in range(n):
          print("%5s" % (lst[i][j]), end=' ')
      print()


def solution():
  p = [0, 0.1, 0.09, 0.06, 0.05, 0.05, 0.04, 0.03]
  q = [0.7, 0.12, 0.07, 0.07, 0.06, 0.05, 0.08, 0.06]

  n = len(p) - 1

  weight = [[0] * (n+1) for _ in range(n+1)]
  cost = [[float('inf')] * (n+1) for _ in range(n+1)]
  root = [[0] * (n+1) for _ in range(n+1)]

  for i in range(n+1):
      weight[i][i] = q[i]
      cost[i][i] = 0

  for x in range(1, n+1):
      for i in range(n+1-x):
          j = i + x
          weight[i][j] = round(weight[i][j-1] + p[j] + q[j], 2)
          # cost
          for k in range(i+1, j+1):
              if cost[i][j] > cost[i][k-1] + cost[k][j] + weight[i][j]:
                  cost[i][j] = round(cost[i][k-1] + cost[k][j] + weight[i][j], 2)
                  root[i][j] = k

  print('weight')
  standard_output(weight)
  print('cost')
  standard_output(cost)
  print('root')
  standard_output(root)


solution()
