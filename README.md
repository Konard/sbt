Мы рассматриваем сбалансированные деревья в определении, данном
китайским школьником Chen Quifeng (Zhongshan Memorial Middle School, Guangdong, China, 2006 год, chenqifeng22@gmail.com).
Оригинальная работа: "[Size Balanced Tree](http://www.scribd.com/doc/3072015/)", [статья в wcipeg вики-энциклопедии](http://wcipeg.com/wiki/Size_Balanced_Tree).

Аналогичную задачу выполняют binary search trees, определенные японскими исследователями Youchi Hirai и Kazuhiko Yamamoto. Статья "Balancing weight-balanced trees", URL: https://yoichihirai.com/bst.pdf (Cambridge University Press, 2011 год)

Работа японских исследователей выглядит внушительно, и их weight-balanced tree выполняет в точности ту же задачу, что и у китайца. Рассматривается большее число случаев взаимного расположения вершин/размеров/весов, и большее число типов вращений (вместо одного типа "rotation" - два типа: "rotation" и "double rotation").
Принцип работы "деревьев японцев" (weight-balanced, WBT) не такой сложный, и при желании можно повторить его. Он реализован внутри некоторых реализаций функциональных языков, таких как Haskell.

Рекомендуется так же рассмотреть реализацию комбинирующую подходы AVL, SBT (Size Balanced Tree) и прошитых деревьев в Links Platform - [SizedAndThreadedAVLBalancedTreeMethods.cs](https://github.com/Konard/LinksPlatform/blob/220ca1201108cf8bc3648e439a1bb253c0968aee/Platform/Platform.Data.Core/Collections/Trees/SizedAndThreadedAVLBalancedTreeMethods.cs)
