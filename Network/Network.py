import torch
import numpy as np
import cv2
import dataset
import models
import utils
import metrics

videos = [i for i in range(10)]
sequence_length = 5
upsample_factor = 4
ds = dataset.SSDataset(64, upsample_factor, videos, sequence_length, transform=dataset.RandomCrop(256))
#data_loader = torch.utils.data.DataLoader(
#     ds, batch_size=1, shuffle=True, num_workers=0)
#data_loader_iter = iter(data_loader)

evaluator = utils.DefaultEvaulator()

FBNet = models.FBNet(upsample_factor)
FBNet.to('cuda')

params = [p for p in FBNet.parameters() if p.requires_grad]
optimizer = torch.optim.Adam(params, lr=1e-4)

ssim = metrics.SSIM()

iter = 600
for i in range(iter):
    print(i)
    item = ds[i]
    item.ToCuda()
    res = FBNet.forward(item)
    
    target = item.target_images[0].unsqueeze(0)
    loss = 1.0 - ssim(target, res)
    optimizer.zero_grad()
    loss.backward()
    optimizer.step()
    print(loss.item())

    res = res.squeeze().cpu().detach()
    res = dataset.ImageTorchToNumpy(res)
    window_name = "Image"
    cv2.imshow(window_name, res)
    cv2.waitKey(0)
cv2.destroyAllWindows()

#evaluator.Plot()
