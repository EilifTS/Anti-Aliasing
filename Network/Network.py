import torch
import numpy as np
import cv2
import dataset
import models
import utils
import metrics
import os

if(__name__ == '__main__'):
    torch.manual_seed(17) # Split the dataset up the same way every time
    videos = torch.randperm(100)
    torch.seed()
    sequence_length = 5
    target_count = 5
    upsample_factor = 4
    data_train = dataset.SSDataset(64, upsample_factor, videos[:10], sequence_length, target_count, transform=dataset.RandomCrop(256))
    data_val = dataset.SSDataset(64, upsample_factor, videos[80:90], sequence_length, target_count, transform=dataset.RandomCrop(256))
    data_test = dataset.SSDataset(64, upsample_factor, videos[90:], sequence_length, target_count, transform=None)
    loader_train = torch.utils.data.DataLoader(data_train, batch_size=1, shuffle=True, num_workers=4, collate_fn=dataset.SSDatasetCollate)
    loader_val = torch.utils.data.DataLoader(data_val, batch_size=1, shuffle=True, num_workers=4, collate_fn=dataset.SSDatasetCollate)
    loader_test = torch.utils.data.DataLoader(data_test, batch_size=1, shuffle=False, num_workers=0, collate_fn=dataset.SSDatasetCollate)


    evaluator = utils.DefaultEvaluator()

    # Create model
    #model_name = 'modelMaster1'
    model_name = 'modelMaster2'
    #model_name = 'modelFB1'
    load_model = False
    start_epoch = 0
    train_losses = []
    val_losses = []
    model = models.MasterNet(upsample_factor)
    loss_function = metrics.MasterLoss()

    # Create optimizer
    params = [p for p in model.parameters() if p.requires_grad]
    optimizer = torch.optim.Adam(params, lr=1e-4)

    # Load model 
    if(load_model):
        checkpoint = torch.load(model_name + "/" + model_name + ".pt")
        model.load_state_dict(checkpoint['model_state_dict'])
        params = [p for p in model.parameters() if p.requires_grad]
        optimizer = torch.optim.Adam(params, lr=1e-4)
        optimizer.load_state_dict(checkpoint['optimizer_state_dict'])
        start_epoch = checkpoint['epoch'] + 1
        train_losses = checkpoint['loss']['train']
        val_losses = checkpoint['loss']['val']
        print("Model loaded")

    model.to('cuda')
    for state in optimizer.state.values():
        for k, v in state.items():
            if torch.is_tensor(v):
                state[k] = v.cuda()
    #optimizer.to('cuda')

    # Create folder to save model related files in
    try:
        os.mkdir(model_name)
        print("Created directory:", model_name)
    except OSError as error:
        print("Directory", model_name, "allready exist")


    epochs = 100

    # Testing
    #utils.VisualizeModel(tus, loader_test)
    #utils.VisualizeMasterModel(model, loader_test)
    #utils.TestModel(model, loader_test)
    #utils.VisualizeDifference(model, loader_test)
    #utils.PlotLosses(train_losses, val_losses)

    for epoch in range(start_epoch, epochs):
        print('Epoch {}'.format(epoch))
        train_loss = utils.TrainEpoch(model, loader_train, optimizer, loss_function)
        val_loss = utils.ValidateModel(model, loader_val, loss_function)
        train_losses.append(train_loss)
        val_losses.append(val_loss)
        print('')

        # Save checkpoint
        print("Saving model")
        torch.save({
            'epoch' : epoch,
            'model_state_dict' : model.state_dict(),
            'optimizer_state_dict' : optimizer.state_dict(),
            'loss' : {'train' : train_losses, 'val' : val_losses}
            }, model_name + "/" + model_name + ".pt")

        # Saving test image
        with torch.no_grad():
            dli = iter(loader_test)
            item = next(dli)
            item.ToCuda()
            res = model.forward(item)[0]
            res = res.squeeze().cpu().detach()
            res = dataset.ImageTorchToNumpy(res)
            cv2.imwrite(model_name + "/temp_img" + str(epoch) + ".png", res)
        #window_name = "Image"
        #cv2.imshow(window_name, res)
        #cv2.waitKey(0)
        #cv2.destroyAllWindows()

        #evaluator.Plot()
