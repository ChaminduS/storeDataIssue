package storeDataIssue

object constants {
    val prfAddrWidth    =   6
    val branchMaskWidth =   4

    val fifo_width      = 1 + prfAddrWidth + branchMaskWidth + 1
    val fifo_depth      = 16 
}

