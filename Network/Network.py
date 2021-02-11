import torch
import numpy as np
import cv2
import dataset
import models
import utils
import metrics
import os

if(__name__ == '__main__'):
    videos = [i for i in range(10)]
    sequence_length = 5
    upsample_factor = 4
    data_train = dataset.SSDataset(64, upsample_factor, videos[:-2], sequence_length, transform=dataset.RandomCrop(256))
    data_val = dataset.SSDataset(64, upsample_factor, videos[-2:-1], sequence_length, transform=dataset.RandomCrop(256))
    data_test = dataset.SSDataset(64, upsample_factor, videos[-1:], sequence_length, transform=None)
    loader_train = torch.utils.data.DataLoader(data_train, batch_size=8, shuffle=True, num_workers=0, collate_fn=dataset.SSDatasetCollate)
    loader_val = torch.utils.data.DataLoader(data_val, batch_size=1, shuffle=True, num_workers=0, collate_fn=dataset.SSDatasetCollate)
    loader_test = torch.utils.data.DataLoader(data_test, batch_size=1, shuffle=False, num_workers=0, collate_fn=dataset.SSDatasetCollate)


    evaluator = utils.DefaultEvaulator()

    # Create model
    model_name = 'modelFB1'
    load_model = True
    start_epoch = 0
    model = models.FBNet(upsample_factor)

    # Create optimizer
    params = [p for p in model.parameters() if p.requires_grad]
    optimizer = torch.optim.Adam(params, lr=1e-4)

    # Load model 
    if(load_model):
        checkpoint = torch.load(model_name + "/" + model_name + ".pt")
        model.load_state_dict(checkpoint['model_state_dict'])
        optimizer.load_state_dict(checkpoint['optimizer_state_dict'])
        start_epoch = checkpoint['epoch'] + 1
        print("Model loaded")

    model.to('cuda')

    # Create folder to save model related files in
    try:
        os.mkdir(model_name)
        print("Created directory:", model_name)
    except OSError as error:
        print("Directory", model_name, "allready exist")

    loss_function = metrics.FBLoss()

    epochs = 100

    # Testing
    with torch.no_grad():
        dli = iter(loader_test)
        for item in dli:
            item.ToCuda()
            res = model.forward(item)
            res = res.squeeze().cpu().detach()
            res = dataset.ImageTorchToNumpy(res)
            window_name = "Image"
            cv2.imshow(window_name, res)
            cv2.waitKey(0)
        cv2.destroyAllWindows()

    if(False):
        for epoch in range(epochs):
            print('Epoch {}'.format(epoch))
            utils.TrainEpoch(model, loader_train, optimizer, loss_function)
            utils.ValidateModel(model, loader_val, loss_function)
            print('')

            # Save checkpoint
            print("Saving model")
            torch.save({
                'epoch' : epoch,
                'model_state_dict' : model.state_dict(),
                'optimizer_state_dict' : optimizer.state_dict(),
                }, model_name + "/" + model_name + ".pt")

            # Saving test image
            with torch.no_grad():
                dli = iter(loader_test)
                item = next(dli)
                item.ToCuda()
                res = model.forward(item)
                res = res.squeeze().cpu().detach()
                res = dataset.ImageTorchToNumpy(res)
                cv2.imwrite(model_name + "/temp_img" + str(epoch) + ".png", res)
        #window_name = "Image"
        #cv2.imshow(window_name, res)
        #cv2.waitKey(0)
        #cv2.destroyAllWindows()

        #evaluator.Plot()
