package storeDataIssue

object constants {
    val prfAddrWidth    =   6
    val branchMaskWidth =   4

    val fifo_width      = 1.U + prfAddrWidth + branchMaskWidth + 1.U
    val fifo_depth      = 16 
}

