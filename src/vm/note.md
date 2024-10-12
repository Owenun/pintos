### design

1. swap分区和交换策略的实现


SubPageTable   存储PageTable的拓展信息
FrameTable     管理所有物理页面
SwapTable      管理swap分区资源


拓展PTE, 存储PageTable额外信息
对于PTE, 当 P == 1, PTE[31:12]表示 paddr
        当 P == 0, PTE[31:12]表示 swapIndex


使用bitmap 管理 Swap分区资源

// swap out
FrameTable

fentry

paddr
&pte

