import torch
import numpy as np
import cv2
import dataset
import models
import utils
import metrics
import os


if(__name__ == '__main__'):
    torch.backends.cudnn.benchmark = True
    #dataset.ConvertPNGDatasetToBMP(None, 4, 100, 60)
    torch.manual_seed(17) # Split the dataset up the same way every time
    videos = torch.randperm(100)
    torch.seed()
    sequence_length = 30 # Number inputs used in the model
    target_indices = [0, 1, 2]#, 15, 31] # The targets the model will calculate loss against, [0] means the last image, [0, 1] the two last etc.
    upsample_factor = 4
    width, height = 1920, 1080
    batch_size = 4
    data_train = dataset.SSDataset(64, upsample_factor, videos[:80], sequence_length, target_indices, transform=dataset.RandomCrop(256, width, height, upsample_factor))
    data_val1 = dataset.SSDataset(64, upsample_factor, videos[80:90], sequence_length, target_indices, transform=dataset.RandomCrop(256, width, height, upsample_factor))
    data_val2 = dataset.SSDataset(64, upsample_factor, videos[80:90], 1, [0], transform=None)
    data_test = dataset.SSDataset(64, upsample_factor, videos[90:], 1, [0], transform=None)
    loader_train = torch.utils.data.DataLoader(data_train, batch_size=batch_size, shuffle=True, num_workers=4, collate_fn=dataset.SSDatasetCollate)
    loader_val1 = torch.utils.data.DataLoader(data_val1, batch_size=batch_size, shuffle=False, num_workers=4, collate_fn=dataset.SSDatasetCollate)
    loader_val2 = torch.utils.data.DataLoader(data_val2, batch_size=1, shuffle=False, num_workers=0, collate_fn=dataset.SSDatasetCollate)
    loader_test = torch.utils.data.DataLoader(data_test, batch_size=1, shuffle=False, num_workers=0, collate_fn=dataset.SSDatasetCollate)

    evaluator = utils.DefaultEvaluator()

    # Create model
    model_name = 'modelMaster'
    load_model = True
    start_epoch = 0
    train_losses = []
    val_epochs = []
    val_losses = []
    val_psnrs = []
    val_ssims = []
    #model = models.FBNet(upsample_factor)
    #model = models.MasterNet(upsample_factor, sequence_length)
    model = models.MasterNet2(upsample_factor)
    #model = models.BicubicModel(upsample_factor)
    #loss_function = metrics.FBLoss()
    loss_function = metrics.MasterLoss2(target_indices)
    
    # Create optimizer
    params = [p for p in model.parameters() if p.requires_grad]
    optimizer = torch.optim.Adam(params, lr=1e-4)

    # Load model 
    if(load_model):
        checkpoint = torch.load(model_name + "/" + model_name + ".pt")
        model.load_state_dict(checkpoint['model_state_dict'])
        params = [p for p in model.parameters() if p.requires_grad]
        optimizer = torch.optim.Adam(params, lr=1e-4*batch_size)
        optimizer.load_state_dict(checkpoint['optimizer_state_dict'])
        start_epoch = checkpoint['epoch'] + 1
        train_losses = checkpoint['loss']['train']
        val_epochs = checkpoint['loss']['val_epochs']
        val_losses = checkpoint['loss']['val']
        val_psnrs = checkpoint['loss']['psnr']
        val_ssims = checkpoint['loss']['ssim']
        print("Model loaded")

    model.to('cuda')
    for state in optimizer.state.values():
        for k, v in state.items():
            if torch.is_tensor(v):
                state[k] = v.cuda()
    


    #utils.SaveModelWeights(model)
    #for g in optimizer.param_groups:
    #    print(g['lr'])
    #    g['lr'] = 1e-5

    print("Model parameters:", sum(p.numel() for p in model.parameters() if p.requires_grad))
    #model.half()

    # Create folder to save model related files in
    try:
        os.mkdir(model_name)
        print("Created directory:", model_name)
    except OSError as error:
        print("Directory", model_name, "allready exist")


    epochs = 100

    # Testing
    #utils.AddGradientHooks(model)
    #utils.CheckMasterModelSampleEff(model, loader_val1, loss_function)
    #utils.VisualizeFBModel(model, loader_test)
    #utils.VisualizeMasterModel(model, loader_test)
    #utils.TestFBModel(model, loader_test)
    #utils.TestMasterModel(model, loader_test)
    #utils.TestMultiple(loader_test)
    #utils.VisualizeDifference(model, loader_test)
    #utils.PlotLosses(train_losses, val_epochs, val_losses, val_psnrs, val_ssims)
    #utils.IllustrateJitterPattern(loader_test, 16, 4)

    for epoch in range(start_epoch, epochs):
        print('Epoch {}'.format(epoch))
        train_loss = utils.TrainEpoch(model, loader_train, optimizer, loss_function)
        train_losses.append(train_loss)
        if(epoch % 1 == 0):
            val_loss, val_psnr, val_ssim = utils.ValidateModel(model, loader_val1, loader_val2, loss_function)
            #val_loss, val_psnr, val_ssim = utils.ValidateFBModel(model, loader_val1, loader_val2, loss_function)
            val_epochs.append(epoch)
            val_losses.append(val_loss)
            val_psnrs.append(val_psnr)
            val_ssims.append(val_ssim)
        

        # Save checkpoint
        print("Saving model")
        #torch.save({
        #    'epoch' : epoch,
        #    'model_state_dict' : model.state_dict(),
        #    'optimizer_state_dict' : optimizer.state_dict(),
        #    'loss' : {'train' : train_losses, 'val' : val_losses}
        #    }, model_name + "/" + model_name + str(epoch) + ".pt")
        
        torch.save({
            'epoch' : epoch,
            'model_state_dict' : model.state_dict(),
            'optimizer_state_dict' : optimizer.state_dict(),
            'loss' : {'train' : train_losses, 'val' : val_losses, 'psnr' : val_psnrs, 'ssim' : val_ssims, 'val_epochs' : val_epochs},
            }, model_name + "/" + model_name + ".pt")

        # Saving test image
        utils.SaveTestImage(model, loader_test, 30, model_name, epoch)
        #utils.SaveTestImageFB(model, loader_test, 30, model_name, epoch)
        print('')

        #window_name = "Image"
        #cv2.imshow(window_name, res)
        #cv2.waitKey(0)
        #cv2.destroyAllWindows()

        #evaluator.Plot()
